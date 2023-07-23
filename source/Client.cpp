/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Sat Jul 22 22:06:29 2023 Francois Michaut
**
** Client.cpp : Implementation of the FileShareProtocol Client
*/

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "FileShare/Client.hpp"
#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/Server.hpp"

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
        std::size_t total = 0;
        std::string_view view;
        Protocol::Request request;
        std::vector<Protocol::Request> result;
        std::size_t ret = 1;

        if (!m_socket.connected())
            return result;
        m_buffer += m_socket.read();
        if (m_buffer.empty()) {
            return result;
        }
        view = m_buffer;
        while (ret != 0) {
            ret = m_protocol.handler().parse_request(view, request);
            if (ret != 0) {
                result.push_back(request);
                total += ret;
                view = view.substr(ret);
            }
        }
        m_buffer = m_buffer.substr(total);
        return result;
    }

    Config Client::default_config()
    {
        return Server::default_config();
    }

    Protocol::Response<void> Client::send_file(std::string filepath, ProgressCallback progress_callback) {
        // TODO: after initial send_file, follow with data_packet calls, calling progress_callback
        std::string msg = m_protocol.handler().format_send_file(m_message_id++, filepath, Utils::HashAlgorithm::SHA512);

        m_socket.write(msg);
        return {};
    }

    Protocol::Response<void> Client::receive_file(std::string filepath, ProgressCallback progress_callback) {
        // TODO: after initial send_file, wait for data_packet calls, calling progress_callback
        std::string msg = m_protocol.handler().format_receive_file(m_message_id++, filepath);

        m_socket.write(msg);
        return {};
    }

    Protocol::Response<Protocol::FileList> Client::list_files(std::string folderpath, std::size_t page_idx) {
        // TODO: get response
        std::string msg = m_protocol.handler().format_list_files(m_message_id++, folderpath, page_idx);

        m_socket.write(msg);
        return {};
    }
}
