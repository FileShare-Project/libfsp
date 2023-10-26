/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Wed Oct 25 21:23:08 2023 Francois Michaut
**
** Client.cpp : Implementation of the FileShareProtocol Client
*/

#include "FileShare/Client.hpp"
#include "FileShare/Errors/TransferErrors.hpp"
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
            std::filesystem::path filepath(data->filepath);

            try {
                m_download_transfers.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(request.message_id),
                    std::forward_as_tuple(m_config.get_downloads_folder() / m_device_uuid / filepath.filename(), data)
                );
            } catch (Errors::Transfer::UpToDateError) {
                return send_reply(request.message_id, Protocol::StatusCode::UP_TO_DATE);
            }
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
        constexpr std::size_t packet_size = 4096; // TODO: find a better place for this, duplicated from ProtocolHandler

        UploadTransferHandler handler(filepath, Utils::HashAlgorithm::SHA512, packet_size);
        std::shared_ptr<Protocol::SendFileData> send_file_data = handler.get_original_request();
        std::uint8_t message_id = send_request(Protocol::CommandCode::SEND_FILE, send_file_data);

        auto result = m_upload_transfers.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(message_id),
            std::forward_as_tuple(std::move(handler))
        );
        UploadTransferHandler &upload_handler = result.first->second;
        const std::optional<Protocol::StatusCode> &status = m_message_queue.get_outgoing_requests().at(message_id).status;

        while (!status.has_value() || status.value() == Protocol::StatusCode::APPROVAL_PENDING) {
            poll_requests();
        }
        if (status.value() != Protocol::StatusCode::STATUS_OK) {
            m_upload_transfers.erase(result.first);
            return {}; // TODO
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

        return {}; // TODO
    }

    Protocol::Response<void> Client::receive_file(std::string filepath, ProgressCallback progress_callback) {
        std::size_t packet_start = 0; // TODO
        std::size_t packet_size = 0; // TODO
        std::shared_ptr<Protocol::ReceiveFileData> receive_file_data = std::make_shared<Protocol::ReceiveFileData>(filepath, packet_size, packet_start);
        std::uint8_t message_id = m_message_queue.send_request({Protocol::CommandCode::RECEIVE_FILE, receive_file_data});
        // TODO: after initial send_file, wait for data_packet calls, calling progress_callback
        std::string msg = m_protocol.handler().format_receive_file(message_id, *receive_file_data);

        m_socket.write(msg);
        return {};
    }

    Protocol::Response<Protocol::FileList> Client::list_files(std::string folderpath, std::size_t page_nb) {
        std::shared_ptr<Protocol::ListFilesData> list_files_data = std::make_shared<Protocol::ListFilesData>(folderpath, page_nb);
        std::uint8_t message_id = send_request(Protocol::CommandCode::LIST_FILES, list_files_data);

        (void)message_id;
        // TODO: get response
        return {};
    }
}
