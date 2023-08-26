/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Sat Aug 26 19:16:47 2023 Francois Michaut
**
** Client.cpp : Implementation of the FileShareProtocol Client
*/

#include "FileShare/Client.hpp"
#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/Protocol/RequestData.hpp"
#include "FileShare/Server.hpp"

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifdef OS_UNIX
    #include <poll.h>
#endif

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
        // Requests will be lost if callers discards them. TODO: improve ?
        m_request_buffer.swap(result);
        return result;
    }

    void Client::poll_requests() {
        std::size_t total = 0;
        std::string_view view;
        Protocol::Request request;
        std::size_t ret = 0;

        if (!m_socket.connected())
            return;
        m_buffer += m_socket.read();
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
        if (request.code == Protocol::CommandCode::RESPONSE) {
            auto data = std::dynamic_pointer_cast<Protocol::ResponseData>(request.request);

            if (m_message_queue.get_outgoing_requests().contains(request.message_id)) {
                receive_reply(request.message_id, data->status);
            }
        } else if (request.code == Protocol::CommandCode::DATA_PACKET) {
            auto data = std::dynamic_pointer_cast<Protocol::DataPacketData>(request.request);

            if (m_download_transfers.contains(data->request_id)) {
                respond_to_request(request, Protocol::StatusCode::STATUS_OK);
            } else {
                respond_to_request(request, Protocol::StatusCode::INVALID_REQUEST_ID);
            }
        } else {
            // The request has not be auto-approved or rejected.
            // It will go through manual approval
            // TODO: send status APPROVAL_PENDING ?
            m_request_buffer.emplace_back(std::move(request));
        }
    }

    void Client::receive_reply(std::uint8_t message_id, Protocol::StatusCode status) {
        // TODO: notify workers ? (upload workers)
        m_message_queue.receive_reply(message_id, status);
    }

    void Client::send_reply(std::uint8_t message_id, Protocol::StatusCode status) {
        auto response_data = std::make_shared<Protocol::ResponseData>(status);
        std::string message = m_protocol.handler().format_response(message_id, status);

        m_message_queue.send_reply(message_id, status);
        m_socket.write(message);
    }

    std::uint8_t Client::send_request(Protocol::CommandCode command, std::shared_ptr<Protocol::IRequestData> request_data) {
        return send_request({command, request_data});
    }

    std::uint8_t Client::send_request(Protocol::Request request) {
        std::uint8_t message_id = m_message_queue.send_request(request);
        std::string message;

        request.message_id = message_id;
        message = m_protocol.handler().format_request(request);
        m_socket.write(message);
        return message_id;
    }

    void Client::respond_to_request(Protocol::Request request, Protocol::StatusCode status) {
        m_message_queue.receive_request(request);
        send_reply(request.message_id, status);

        if (status != Protocol::StatusCode::STATUS_OK)
            return;

        if (request.code == Protocol::CommandCode::SEND_FILE) {
            std::shared_ptr<Protocol::SendFileData> data = std::dynamic_pointer_cast<Protocol::SendFileData>(request.request);
            std::filesystem::path filepath(data->filepath);

            m_download_transfers.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(request.message_id),
                std::forward_as_tuple(m_config.get_downloads_folder() / m_device_uuid / filepath.filename(), data)
            );
        } else if (request.code == Protocol::CommandCode::DATA_PACKET) {
            auto data = std::dynamic_pointer_cast<Protocol::DataPacketData>(request.request);

            m_download_transfers.at(data->request_id).receive_packet(*data);
        }
    }

    Config Client::default_config() {
        return Server::default_config();
    }

    Protocol::StatusCode Client::wait_for_status(std::uint8_t message_id) {
        auto &message = m_message_queue.get_outgoing_requests().at(message_id);

        // TODO HACK: this is not good as we could wait forever if peer is
        // trying to DDOS us by not sending a response ever.
        // Implement async model instead
        while (!message.status.has_value()) {
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

    Protocol::Response<void> Client::send_file(std::string filepath, ProgressCallback progress_callback) {
        constexpr std::size_t packet_size = 4096; // TODO: find a better place for this, duplicated from ProtocolHandler

        // TODO: move this logic in a worker class
        std::string file_hash = Utils::file_hash(Utils::HashAlgorithm::SHA512, filepath);
        std::filesystem::file_time_type file_updated_at = std::filesystem::last_write_time(filepath);
        std::size_t file_size = std::filesystem::file_size(filepath);
        std::size_t total_packets = file_size / packet_size + (file_size % packet_size == 0 ? 0 : 1);
        std::shared_ptr<Protocol::SendFileData> send_file_data = std::make_shared<Protocol::SendFileData>(filepath, Utils::HashAlgorithm::SHA512, file_hash, file_updated_at, packet_size, total_packets);

        std::uint8_t message_id = send_request(Protocol::CommandCode::SEND_FILE, send_file_data);

        if (wait_for_status(message_id) != Protocol::StatusCode::STATUS_OK) {
            return {}; // TODO
        }
        progress_callback(filepath, 0, file_size);

        std::ifstream file(filepath);
        std::size_t packet_id = 0;
        std::uint8_t request_id = message_id;

        file.exceptions(std::ifstream::badbit); // Enable exceptions on IO operations
        while (packet_id < total_packets) {
            std::uint8_t available = m_message_queue.available_send_slots();

            if (available > 0) {
                std::vector<std::uint8_t> ids;
                std::size_t remaining = total_packets - packet_id;

                ids.reserve(available);
                for (std::uint8_t i = 0; i < available && i < remaining; i++) {
                    std::array<char, packet_size> buffer;

                    file.read(buffer.data(), packet_size);

                    std::string data(buffer.data(), file.gcount());
                    auto data_packet_data = std::make_shared<Protocol::DataPacketData>(request_id, packet_id++, data);
                    std::uint8_t message_id = send_request(Protocol::CommandCode::DATA_PACKET, data_packet_data);

                    ids.push_back(message_id);
                }
                for (auto &id : ids) {
                    // TODO: implement retries here
                    if (wait_for_status(id) != Protocol::StatusCode::STATUS_OK) {
                        return {}; // TODO
                    }
                }
                // TODO: this is not exact : last packet might have a smaller size
                progress_callback(filepath, packet_id * packet_size, file_size);
            }
            poll_requests(); // TODO: currently blocking, but if it changes, needs to add a poll() call to avoid spamming
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
