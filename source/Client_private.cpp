/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Mon Oct 23 21:33:10 2023 Francois Michaut
** Last update Thu Nov  9 12:45:22 2023 Francois Michaut
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
        // TODO: implement permission logic with allowed / denied files in the Config
        // TODO: implement retries logic
        switch (request.code) {
            case Protocol::CommandCode::RESPONSE: {
                auto data = std::dynamic_pointer_cast<Protocol::ResponseData>(request.request);

                if (m_message_queue.get_outgoing_requests().contains(request.message_id)) {
                    receive_reply(request.message_id, data->status);
                }
                break;
            }
            case Protocol::CommandCode::DATA_PACKET: {
                auto data = std::dynamic_pointer_cast<Protocol::DataPacketData>(request.request);

                if (m_download_transfers.contains(data->request_id)) {
                    respond_to_request(request, Protocol::StatusCode::STATUS_OK);
                } else {
                    respond_to_request(request, Protocol::StatusCode::INVALID_REQUEST_ID);
                }
                break;
            }
            case Protocol::CommandCode::SEND_FILE: {
                // detect this is a send file in reply to a RECEIVE_FILE, and auto-accept
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
                    m_message_queue.receive_reply(original_request->first, Protocol::StatusCode::STATUS_OK);
                    respond_to_request(request, Protocol::StatusCode::STATUS_OK);
                    break;
                }
                // fallthrough default (manual approval) if no matching requests where found
            }
            default:
                // The request has not be auto-approved or rejected.
                // It will go through manual approval
                respond_to_request(request, Protocol::StatusCode::APPROVAL_PENDING);
                m_request_buffer.emplace_back(std::move(request));
        }
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

        if (source_request.code == Protocol::CommandCode::DATA_PACKET) {
            auto packet_data = std::dynamic_pointer_cast<Protocol::DataPacketData>(source_request.request);
            auto &handler = m_upload_transfers.at(packet_data->request_id);
            std::shared_ptr<Protocol::DataPacketData> new_packet = handler.get_next_packet(packet_data->request_id);

            if (new_packet) {
                send_request(Protocol::CommandCode::DATA_PACKET, new_packet);
            }
        } else if (source_request.code == Protocol::CommandCode::SEND_FILE) {
            auto &handler = m_upload_transfers.at(message_id);

            // TODO: do not rely on that hardcoded 5
            for (int i = 0; i < 5 && !handler.finished() && m_message_queue.available_send_slots() > 0; i++) {
                std::shared_ptr<Protocol::DataPacketData> new_packet = handler.get_next_packet(message_id);

                send_request(Protocol::CommandCode::DATA_PACKET, new_packet);
            }
        }
    }

    void Client::send_reply(Protocol::MessageID message_id, Protocol::StatusCode status) {
        auto response_data = std::make_shared<Protocol::ResponseData>(status);
        std::string message = m_protocol.handler().format_response(message_id, status);

        m_message_queue.send_reply(message_id, status);
        m_socket.write(message);
    }

    Protocol::MessageID Client::send_request(Protocol::CommandCode command, std::shared_ptr<Protocol::IRequestData> request_data) {
        return send_request({command, request_data});
    }

    Protocol::MessageID Client::send_request(Protocol::Request request) {
        Protocol::MessageID message_id = m_message_queue.send_request(request);
        std::string message;

        request.message_id = message_id;
        message = m_protocol.handler().format_request(request);
        m_socket.write(message);
        return message_id;
    }

    Client::UploadTransferMap::iterator Client::create_upload(std::string filepath) {
        constexpr std::size_t packet_size = 4096; // TODO: find a better place for this, duplicated from ProtocolHandler

        return create_upload(std::move(filepath), 4096, 0);
    }

    Client::UploadTransferMap::iterator Client::create_upload(std::string filepath, std::size_t packet_size, std::size_t packet_start) {
        UploadTransferHandler handler(filepath, Utils::HashAlgorithm::SHA512, packet_size);
        std::shared_ptr<Protocol::SendFileData> send_file_data = handler.get_original_request();
        Protocol::MessageID message_id = send_request(Protocol::CommandCode::SEND_FILE, send_file_data);

        auto result = m_upload_transfers.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(message_id),
            std::forward_as_tuple(std::move(handler))
        );
        return result.first;
    }

    Client::DownloadTransferMap::iterator Client::create_download(Protocol::MessageID request_id, const std::shared_ptr<Protocol::SendFileData> &data) {
        std::filesystem::path filepath(data->filepath); // TODO: add filepath root translation
        auto result = m_download_transfers.end();

        try {
            result = m_download_transfers.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(request_id),
                std::forward_as_tuple(m_config.get_downloads_folder() / m_device_uuid / filepath.filename(), data)
                ).first;
            send_reply(request_id, Protocol::StatusCode::STATUS_OK);
        } catch (Errors::Transfer::UpToDateError) {
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
        while (!message.status.has_value() || message.status.value() == Protocol::StatusCode::APPROVAL_PENDING) {
            std::array<struct pollfd, 1> fds;
            nfds_t nfds = 1;
            int nb_ready = 0;

            fds[0] = {m_socket.get_fd(), POLLIN, 0};
            nb_ready = ppoll(fds.data(), nfds, nullptr, nullptr); // TODO: add timeout
            if (nb_ready < 0) // TODO: handle signals
                throw std::runtime_error("Failed to poll for status");
            poll_requests();
            if (!m_socket.connected())
                throw std::runtime_error("connection lost while waiting for status");
        }
        return message.status.value();
    }
}
