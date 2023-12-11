/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Sun Dec 10 18:46:14 2023 Francois Michaut
**
** Client.cpp : Implementation of the FileShareProtocol Client
*/

#include "FileShare/Client.hpp"
#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/Protocol/RequestData.hpp"
#include "FileShare/Server.hpp"

#include <cstdio>
#include <stdexcept>


// TODO: Use custom classes for exceptions
namespace FileShare {
    Client::Client(const CppSockets::IEndpoint &peer, std::string device_uuid, std::string public_key, Protocol::Protocol protocol, Config config) :
        m_config(std::move(config)), m_protocol(std::move(protocol)),
        m_device_uuid(device_uuid), m_public_key(public_key)
    {
        reconnect(peer);
    }

    Client::Client(CppSockets::TlsSocket &&peer, std::string device_uuid, std::string public_key, Protocol::Protocol protocol, Config config) :
        m_config(std::move(config)), m_protocol(std::move(protocol)),
        m_device_uuid(device_uuid), m_public_key(public_key)
    {
        reconnect(std::move(peer));
    }

    const Config &Client::get_config() const {
        return m_config;
    }

    void Client::set_config(Config config) {
        m_config = std::move(config);
    }

    const CppSockets::TlsSocket &Client::get_socket() const {
        return m_socket;
    }

    void Client::reconnect(const CppSockets::IEndpoint &peer) {
        m_socket = CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0); // TODO check if needed
        m_socket.connect(peer);
        // TODO authentification + protocol handshake
    }

    void Client::reconnect(CppSockets::TlsSocket &&peer) {
        if (!peer.connected()) {
            throw std::runtime_error("Socket is not connected");
        }
        m_socket = std::move(peer);
        // TODO authentification + protocol handshake
    }

    std::vector<Protocol::Request> Client::pull_requests() {
        std::vector<Protocol::Request> result;

        poll_requests();
        // Move the buffer in the result, and clears the buffer
        // Requests will be lost if callers discards them. TODO: improve ? Could clear when user call `respond_to_request()`
        m_request_buffer.swap(result);
        return result;
    }

    void Client::respond_to_request(Protocol::Request request, Protocol::StatusCode status) {
        m_message_queue.receive_request(request);

        if (status != Protocol::StatusCode::STATUS_OK) {
            send_reply(request.message_id, status);
            return;
        }

        switch (request.code) {
            case Protocol::CommandCode::DATA_PACKET: {
                auto data = std::dynamic_pointer_cast<Protocol::DataPacketData>(request.request);
                auto &handler = m_download_transfers.at(data->request_id);

                handler.receive_packet(*data);
                if (handler.finished()) {
                    m_download_transfers.erase(data->request_id);
                }
                break;
            }
            case Protocol::CommandCode::SEND_FILE: {
                std::shared_ptr<Protocol::SendFileData> data = std::dynamic_pointer_cast<Protocol::SendFileData>(request.request);

                create_download(request.message_id, data);
                return; // create_download already sends reply to request

            }
            case Protocol::CommandCode::RECEIVE_FILE: {
                std::shared_ptr<Protocol::ReceiveFileData> data = std::dynamic_pointer_cast<Protocol::ReceiveFileData>(request.request);
                auto virtual_path = data->filepath;
                auto host_path = m_config.get_file_mapping().virtual_to_host(virtual_path);
                auto [handler, status] = prepare_upload(host_path.string(), virtual_path, data->packet_size, data->packet_start);

                // Send reply to original RECEIVE_FILE before we send SEND_FILE
                send_reply(request.message_id, status);
                if (status == Protocol::StatusCode::STATUS_OK) {
                    create_upload(std::move(handler.value()));
                }
                return;
            }
            case Protocol::CommandCode::LIST_FILES: {
                auto data = std::dynamic_pointer_cast<Protocol::ListFilesData>(request.request);
                constexpr std::size_t packet_size = 4096; // TODO: find a better place for this, duplicated from ProtocolHandler
                ListFilesTransferHandler handler(data->folderpath, m_config.get_file_mapping(), packet_size);

                auto result = m_list_files_transfers.emplace(std::piecewise_construct, std::forward_as_tuple(request.message_id), std::forward_as_tuple(std::move(handler)));

                send_reply(request.message_id, Protocol::StatusCode::STATUS_OK);
                send_request(Protocol::CommandCode::FILE_LIST, result.first->second.get_next_packet(request.message_id));
                return;
            }
            case Protocol::CommandCode::FILE_LIST: {
                auto data = std::dynamic_pointer_cast<Protocol::FileListData>(request.request);
                auto &handler = m_file_list_transfers.at(data->request_id);

                handler.receive_packet(*data);
                break;
            }
            default:
                break;
        }
        send_reply(request.message_id, Protocol::StatusCode::STATUS_OK);
    }

    Config Client::default_config() {
        return Server::default_config();
    }

    Protocol::Response<void> Client::send_file(std::string filepath, ProgressCallback progress_callback) {
        auto result = create_host_upload(filepath);
        Protocol::MessageID message_id = result->first;
        UploadTransferHandler &upload_handler = result->second;
        Protocol::StatusCode status = wait_for_status(message_id);

        // TODO: handle APPROVAL_PENDING
        if (status != Protocol::StatusCode::STATUS_OK) {
            m_upload_transfers.erase(result);
            return {status, {}};
        }
        while (!upload_handler.finished()) {
            if (m_message_queue.available_send_slots() > 0) {
                auto packet = upload_handler.get_next_packet(message_id);

                // TODO: monitor responses of theses packets
                send_request(Protocol::CommandCode::DATA_PACKET, packet);
                progress_callback(filepath, upload_handler.get_current_size(), upload_handler.get_total_size());
            } else {
                // TODO: currently blocking, but if it changes, needs to add a poll() call to avoid spamming loop
                poll_requests();
            }
        }

        return {status, {}}; // TODO
    }

    Protocol::Response<void> Client::receive_file(std::string filepath, ProgressCallback progress_callback) {
        std::size_t packet_start = 0; // TODO
        std::size_t packet_size = 0; // TODO
        std::shared_ptr<Protocol::ReceiveFileData> receive_file_data = std::make_shared<Protocol::ReceiveFileData>(filepath, packet_size, packet_start);
        Protocol::MessageID message_id = send_request(Protocol::CommandCode::RECEIVE_FILE, receive_file_data);
        Protocol::StatusCode status = wait_for_status(message_id);

        // TODO: handle APPROVAL_PENDING
        if (status != Protocol::StatusCode::STATUS_OK) {
            return {status, {}};
        }

        auto incomming_requests = m_message_queue.get_incomming_requests();
        auto request = std::find_if(incomming_requests.begin(), incomming_requests.end(), [&filepath](const auto &item) {
            if (item.second.request.code != Protocol::CommandCode::SEND_FILE) {
                return false;
            }

            auto original_data = std::dynamic_pointer_cast<Protocol::SendFileData>(item.second.request.request);
            return filepath == original_data->filepath;
        });
        if (request == incomming_requests.end() || !m_download_transfers.contains(request->first)) {
            throw std::runtime_error("Failed to locate download transfer"); // TODO: we got STATUS_OK but no incomming request ? Something is wrong
        }
        auto &transfer_handler = m_download_transfers.at(request->first);

        while (!transfer_handler.finished()) {
            progress_callback(filepath, transfer_handler.get_current_size(), transfer_handler.get_total_size());
            poll_requests(); // TODO: currently blocking, but if it changes, needs to add a poll() call to avoid spamming loop
        }

        return {status, {}}; // TODO
    }

    Protocol::Response<std::vector<Protocol::FileInfo>> Client::list_files(std::string folderpath) {
        std::shared_ptr<Protocol::ListFilesData> list_files_data = std::make_shared<Protocol::ListFilesData>(folderpath);
        Protocol::MessageID message_id = send_request(Protocol::CommandCode::LIST_FILES, list_files_data);
        Protocol::StatusCode status = wait_for_status(message_id);

        if (status != Protocol::StatusCode::STATUS_OK) {
            return {status, {}};
        }
        auto iter = m_file_list_transfers.find(message_id);

        if (iter == m_file_list_transfers.end()) {
            std::runtime_error("Failed to locate list file transfer"); // TODO: we got STATUS_OK but no transfer? something is wrong.
        }
        auto &handler = iter->second;

        while (!handler.finished()) {
            poll_requests(); // TODO: currently blocking, but if it changes, needs to add a poll() call to avoid spamming loop
        }

        auto file_list = std::make_shared<std::vector<Protocol::FileInfo>>(handler.get_file_list());

        m_file_list_transfers.erase(message_id);
        return {status, file_list};
    }
}

// TODO: there are many places where client will loop forever if socket is not connected, fix it.
