/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Tue Jul 18 13:44:02 2023 Francois Michaut
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
        m_socket(CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0)), m_config(std::move(config)),
        m_protocol(std::move(protocol)), m_device_uuid(device_uuid), m_public_key(public_key)
    {
        reconnect(peer);
    }

    Client::Client(CppSockets::TlsSocket &&peer, std::string device_uuid, std::string public_key, Protocol::Protocol protocol, Config config) :
        m_socket(CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0)), m_config(std::move(config)),
        m_protocol(std::move(protocol)), m_device_uuid(device_uuid), m_public_key(public_key)
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

    Config Client::default_config()
    {
        return Server::default_config();
    }
}
