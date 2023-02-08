/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Tue Feb  7 22:41:08 2023 Francois Michaut
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

#include "FileShareProtocol/Client.hpp"
#include "FileShareProtocol/Server.hpp"

// TODO: Use custom classes for exceptions
namespace FileShareProtocol {
    Client::Client(const CppSockets::IEndpoint &peer, Config config) :
        socket(CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0)), config(std::move(config))
    {
        reconnect(peer);
    }

    Client::Client(CppSockets::TlsSocket &&peer, Config config) :
        socket(CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0)), config(std::move(config))
    {
        reconnect(std::move(peer));
    }

    const Config &Client::get_config() const {
        return config;
    }

    void Client::set_config(const Config &config) {
        this->config = config;
    }

    const CppSockets::TlsSocket &Client::get_socket() const {
        return socket;
    }

    void Client::reconnect(const CppSockets::IEndpoint &peer) {
        socket = CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0); // TODO check if needed
        socket.connect(peer);
        // TODO authentification + protocol handshake
    }

    void Client::reconnect(CppSockets::TlsSocket &&peer) {
        if (!peer.connected()) {
            throw std::runtime_error("Socket is not connected");
        }
        socket = std::move(peer);
        // TODO authentification + protocol handshake
    }

    Config Client::default_config()
    {
        return Server::default_config();
    }
}
