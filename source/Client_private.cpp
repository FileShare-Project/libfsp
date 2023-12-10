/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Mon Oct 23 21:33:10 2023 Francois Michaut
** Last update Sun Dec 10 18:47:02 2023 Francois Michaut
**
** Client_private.cpp : Private functions of Client implementation
*/

#include "FileShare/Client.hpp"
#include "FileShare/Errors/TransferErrors.hpp"

#ifdef OS_UNIX
    #include <poll.h>
#endif

namespace FileShare {
    void Client::poll_requests() {
        std::size_t total = 0;
        std::string_view view;
        Protocol::Request request;
        std::size_t ret = 0;

        if (!m_socket.connected())
            return;
        m_buffer += m_socket.read(); // TODO: add a timeout
        if (m_buffer.empty()) {
            return;
        }
        view = m_buffer;
        while (true) {
            ret = m_protocol.handler().parse_request(view, request);
            if (ret == 0) {
                break;
            }
            authorize_request(request);
            total += ret;
            view = view.substr(ret);
        }
        m_buffer = m_buffer.substr(total);
    }

    void Client::authorize_request(Protocol::Request request) {
        // TODO: implement retries logic
        switch (request.code) {
            case Protocol::CommandCode::RESPONSE: {
                auto data = std::dynamic_pointer_cast<Protocol::ResponseData>(request.request);

                if (m_message_queue.get_outgoing_requests().contains(request.message_id)) {
                    receive_reply(request.message_id, data->status);
                }
                return;
            }
            case Protocol::CommandCode::DATA_PACKET: {
                auto data = std::dynamic_pointer_cast<Protocol::DataPacketData>(request.request);

                if (m_download_transfers.contains(data->request_id)) {
                    respond_to_request(request, Protocol::StatusCode::STATUS_OK);
                } else {
                    respond_to_request(request, Protocol::StatusCode::INVALID_REQUEST_ID);
                }
                return;
            }
            case Protocol::CommandCode::SEND_FILE: {
                // detect this is a send file in reply to a RECEIVE_FILE we sent, and auto-accept
                auto data = std::dynamic_pointer_cast<Protocol::SendFileData>(request.request);
                auto outgoing_requests = m_message_queue.get_outgoing_requests();
                auto original_request = std::find_if(outgoing_requests.begin(), outgoing_requests.end(), [&data](const auto &item) {
                    if (item.second.request.code != Protocol::CommandCode::RECEIVE_FILE) {
                        return false;
                    }

                    auto original_data = std::dynamic_pointer_cast<Protocol::ReceiveFileData>(item.second.request.request);
                    return data->filepath == original_data->filepath;
                });

                if (original_request != outgoing_requests.end()) {
                    // We received a SEND_FILE to our RECEIVE_FILE -> we can mark is as OK since we don't need to keep it anymore
                    m_message_queue.receive_reply(original_request->first, Protocol::StatusCode::STATUS_OK);
                    respond_to_request(request, Protocol::StatusCode::STATUS_OK);
                    return;
                }
                break; // fallthrough default (manual approval) if no matching requests where found
            }
            case Protocol::CommandCode::RECEIVE_FILE: {
                // respond_to_request will handle inexistant / forbidden paths
                return respond_to_request(request, Protocol::StatusCode::STATUS_OK);
            }
            case Protocol::CommandCode::LIST_FILES: {
                auto data = std::dynamic_pointer_cast<Protocol::ListFilesData>(request.request);
                auto &file_mapping = m_config.get_file_mapping();
                auto node = file_mapping.find_virtual_node(data->folderpath);

                if (!node.has_value() || (node->get_type() != PathNode::VIRTUAL && file_mapping.is_forbidden(node->get_host_path()))) {
                    return respond_to_request(request, Protocol::StatusCode::FILE_NOT_FOUND);
                }
                return respond_to_request(request, Protocol::StatusCode::STATUS_OK);
            }
            case Protocol::CommandCode::FILE_LIST: {
                auto data = std::dynamic_pointer_cast<Protocol::FileListData>(request.request);

                if (m_file_list_transfers.contains(data->request_id)) {
                    respond_to_request(request, Protocol::StatusCode::STATUS_OK);
                } else {
                    respond_to_request(request, Protocol::StatusCode::INVALID_REQUEST_ID);
                }
                return;
             }
            default:
                break; // Exit the switch but continue with the default APPROVAL_PENDING code.
        }
        // The request has not be auto-approved or rejected.
        // It will go through manual approval
        respond_to_request(request, Protocol::StatusCode::APPROVAL_PENDING);
        m_request_buffer.emplace_back(std::move(request));
    }

    void Client::receive_reply(Protocol::MessageID message_id, Protocol::StatusCode status) {
        auto source_request = m_message_queue.get_outgoing_requests().at(message_id).request;

        if (status == Protocol::StatusCode::STATUS_OK && source_request.code == Protocol::CommandCode::RECEIVE_FILE) {
            // Peer accepted our request; but we will mark it as PENDING since we are waiting on the SEND_FILE packet
            m_message_queue.receive_reply(message_id, Protocol::StatusCode::APPROVAL_PENDING);
            return;
        }
        m_message_queue.receive_reply(message_id, status);
        if (status != Protocol::StatusCode::STATUS_OK) {
            return;
        }

        switch (source_request.code) {
            case Protocol::CommandCode::DATA_PACKET: {
                auto packet_data = std::dynamic_pointer_cast<Protocol::DataPacketData>(source_request.request);
                auto &handler = m_upload_transfers.at(packet_data->request_id);
                std::shared_ptr<Protocol::DataPacketData> new_packet = handler.get_next_packet(packet_data->request_id);

                if (new_packet) {
                    send_request(Protocol::CommandCode::DATA_PACKET, new_packet);
                } else {
                    m_upload_transfers.erase(packet_data->request_id);
                }
                break;
            }
            case Protocol::CommandCode::SEND_FILE: {
                auto &handler = m_upload_transfers.at(message_id);

                // TODO: do not rely on that hardcoded 5
                for (int i = 0; i < 5 && !handler.finished() && m_message_queue.available_send_slots() > 0; i++) {
                    std::shared_ptr<Protocol::DataPacketData> new_packet = handler.get_next_packet(message_id);

                    send_request(Protocol::CommandCode::DATA_PACKET, new_packet);
                }
                if (handler.finished()) {
                    m_upload_transfers.erase(message_id);
                }
                break;
            }
            case Protocol::CommandCode::LIST_FILES: {
                m_file_list_transfers.try_emplace(message_id);
                break;
            }
            case Protocol::CommandCode::FILE_LIST: {
                auto data = std::dynamic_pointer_cast<Protocol::FileListData>(source_request.request);
                auto &handler = m_list_files_transfers.at(data->request_id);
                auto new_packet = handler.get_next_packet(data->request_id);

                if (new_packet) {
                    send_request(Protocol::CommandCode::FILE_LIST, new_packet);
                } else {
                    m_upload_transfers.erase(message_id);
                }
                break;
            }
            default:
                break;
        }
    }

    void Client::send_reply(Protocol::MessageID message_id, Protocol::StatusCode status) {
        auto response_data = std::make_shared<Protocol::ResponseData>(status);
        std::string message = m_protocol.handler().format_response(message_id, status);

        m_message_queue.send_reply(message_id, status);
        m_socket.write(message);
    }

    Protocol::MessageID Client::send_request(Protocol::CommandCode command, std::shared_ptr<Protocol::IRequestData> request_data) {
        return send_request({command, request_data, 0});
    }

    Protocol::MessageID Client::send_request(Protocol::Request request) {
        Protocol::MessageID message_id = m_message_queue.send_request(request);
        std::string message;

        request.message_id = message_id;
        message = m_protocol.handler().format_request(request);
        m_socket.write(message);
        return message_id;
    }

    std::pair<std::optional<UploadTransferHandler>, Protocol::StatusCode> Client::prepare_upload(std::filesystem::path host_filepath, std::string virtual_filepath, std::size_t packet_size, std::size_t packet_start) {
        std::optional<UploadTransferHandler> handler;

        if (host_filepath.empty() || m_config.get_file_mapping().is_forbidden(host_filepath)) {
            return std::make_pair(std::move(handler), Protocol::StatusCode::FILE_NOT_FOUND);
        }

        std::string file_hash = Utils::file_hash(Utils::HashAlgorithm::SHA512, host_filepath);
        std::filesystem::directory_entry entry(host_filepath);
        std::filesystem::file_time_type file_updated_at = entry.last_write_time();
        std::size_t file_size = entry.file_size();
        std::size_t total_packets = file_size / packet_size + (file_size % packet_size == 0 ? 0 : 1);
        // TODO: add FILE_NOT_FOUND errors if file does not exists on filesystem

        if (packet_start > total_packets) {
            return std::make_pair(std::move(handler), Protocol::StatusCode::BAD_REQUEST);
        }
        total_packets -= packet_start;

        std::shared_ptr<Protocol::SendFileData> send_file_data = std::make_shared<Protocol::SendFileData>(std::move(virtual_filepath), Utils::HashAlgorithm::SHA512, file_hash, file_updated_at, packet_size, total_packets);
        handler = {std::move(host_filepath), std::move(send_file_data), packet_start};
        return std::make_pair(std::move(handler), Protocol::StatusCode::STATUS_OK);
    }

    Client::UploadTransferMap::iterator Client::create_host_upload(std::filesystem::path host_filepath) {
        constexpr std::size_t packet_size = 4096; // TODO: find a better place for this, duplicated from ProtocolHandler
        auto virtual_path = m_config.get_file_mapping().host_to_virtual(host_filepath);

        if (virtual_path.empty())
            virtual_path = host_filepath.filename();
        auto result = prepare_upload(host_filepath.string(), virtual_path.string(), packet_size, 0);

        if (result.second == Protocol::StatusCode::STATUS_OK) {
            return create_upload(std::move(result.first.value()));
        }
        throw std::runtime_error("Failed to send file '" + host_filepath.string() + "': " + Protocol::status_to_str(result.second));
    }

    Client::UploadTransferMap::iterator Client::create_upload(UploadTransferHandler handler) {
        Protocol::MessageID message_id = send_request(Protocol::CommandCode::SEND_FILE, handler.get_original_request());

        auto result = m_upload_transfers.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(message_id),
            std::forward_as_tuple(std::move(handler))
        );
        return result.first;
    }

    Client::DownloadTransferMap::iterator Client::create_download(Protocol::MessageID request_id, const std::shared_ptr<Protocol::SendFileData> &data) {
        std::filesystem::path filepath(data->filepath);
        auto result = m_download_transfers.end();

        try {
            result = m_download_transfers.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(request_id),
                std::forward_as_tuple(m_config.get_downloads_folder() / m_device_uuid / filepath.relative_path(), data)
            ).first;
            send_reply(request_id, Protocol::StatusCode::STATUS_OK);
        } catch (Errors::Transfer::UpToDateError &) {
            send_reply(request_id, Protocol::StatusCode::UP_TO_DATE);
        }
        return result;
    }

    // TODO: deprecate
    Protocol::StatusCode Client::wait_for_status(Protocol::MessageID message_id) {
        auto &message = m_message_queue.get_outgoing_requests().at(message_id);

        // TODO HACK: this is not good as we could wait forever if peer is
        // trying to DDOS us by not sending a response ever.
        // Implement async model instead
        while ((!message.status.has_value() || message.status.value() == Protocol::StatusCode::APPROVAL_PENDING) && m_socket.connected()) {
            std::array<struct pollfd, 1> fds;
            nfds_t nfds = 1;
            int nb_ready = 0;

            fds[0] = {m_socket.get_fd(), POLLIN, 0};
#ifdef OS_APPLE
            nb_ready = poll(fds.data(), nfds, -1); // TODO: add timeout
#else
            nb_ready = ppoll(fds.data(), nfds, nullptr, nullptr); // TODO: add timeout
#endif
            if (nb_ready < 0) // TODO: handle signals
                throw std::runtime_error("Failed to poll for status");
            poll_requests();
            if (!m_socket.connected())
                throw std::runtime_error("connection lost while waiting for status");
        }
        return message.status.value();
    }
}
