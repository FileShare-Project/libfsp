/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Thu Nov  9 20:01:37 2023 Francois Michaut
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

        if (request.code == Protocol::CommandCode::SEND_FILE) {
            std::shared_ptr<Protocol::SendFileData> data = std::dynamic_pointer_cast<Protocol::SendFileData>(request.request);

            create_download(request.message_id, data);
            return; // create_download already sends reply to request
        } else if (request.code == Protocol::CommandCode::RECEIVE_FILE) {
            std::shared_ptr<Protocol::ReceiveFileData> data = std::dynamic_pointer_cast<Protocol::ReceiveFileData>(request.request);
            std::filesystem::path filepath(data->filepath); // TODO: add filepath root translation

            // Send reply to original RECEIVE_FILE before SEND_FILE
            send_reply(request.message_id, Protocol::StatusCode::STATUS_OK);
            create_upload(filepath, data->packet_size, data->packet_start);
            return;
        } else if (request.code == Protocol::CommandCode::DATA_PACKET) {
            auto data = std::dynamic_pointer_cast<Protocol::DataPacketData>(request.request);

            m_download_transfers.at(data->request_id).receive_packet(*data);
        }
        send_reply(request.message_id, Protocol::StatusCode::STATUS_OK);
    }

    Config Client::default_config() {
        return Server::default_config();
    }

    Protocol::Response<void> Client::send_file(std::string filepath, ProgressCallback progress_callback) {
        // TODO: add filepath root translation
        auto result = create_upload(filepath);
        Protocol::MessageID message_id = result->first;
        UploadTransferHandler &upload_handler = result->second;
        Protocol::StatusCode status = wait_for_status(message_id);

        // TODO: handle APPROVAL_PENDING
        if (status != Protocol::StatusCode::STATUS_OK) {
            m_upload_transfers.erase(result);
            return {status};
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

        return {status}; // TODO
    }

    Protocol::Response<void> Client::receive_file(std::string filepath, ProgressCallback progress_callback) {
        std::size_t packet_start = 0; // TODO
        std::size_t packet_size = 0; // TODO
        std::shared_ptr<Protocol::ReceiveFileData> receive_file_data = std::make_shared<Protocol::ReceiveFileData>(filepath, packet_size, packet_start);
        Protocol::MessageID message_id = send_request(Protocol::CommandCode::RECEIVE_FILE, receive_file_data);
        Protocol::StatusCode status = wait_for_status(message_id);

        // TODO: handle APPROVAL_PENDING
        if (status != Protocol::StatusCode::STATUS_OK) {
            return {status};
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
            return {status};
        }
        auto &transfer_handler = m_download_transfers.at(request->first);

        while (!transfer_handler.finished()) {
            progress_callback(filepath, transfer_handler.get_current_size(), transfer_handler.get_total_size());
            poll_requests(); // TODO: currently blocking, but if it changes, needs to add a poll() call to avoid spamming loop
        }

        return {status}; // TODO
    }

    Protocol::Response<Protocol::FileList> Client::list_files(std::string folderpath, std::size_t page_nb) {
        std::shared_ptr<Protocol::ListFilesData> list_files_data = std::make_shared<Protocol::ListFilesData>(folderpath, page_nb);
        Protocol::MessageID message_id = send_request(Protocol::CommandCode::LIST_FILES, list_files_data);

        (void)message_id;
        // TODO: get response
        return {};
    }
}
