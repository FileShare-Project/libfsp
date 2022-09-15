/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Wed Sep 14 22:21:43 2022 Francois Michaut
**
** Client.cpp : Implementation of the FileShareProtocol Client
*/

#include "FileShareProtocol/Client.hpp"

#include <stdexcept>

// TODO: Use custom classes for exceptions
namespace FileShareProtocol {
    inline static CppSockets::Socket make_socket_from_endpoint(const CppSockets::IEndpoint &peer) {
        auto socket = CppSockets::Socket(AF_INET, SOCK_STREAM, 0);

        socket.connect(peer);
        return std::move(socket);
    }

    Client::Client(const CppSockets::IEndpoint &peer, ClientConfig config) :
        socket(CppSockets::Socket(AF_INET, SOCK_STREAM, 0)), config(std::move(config))
    {
        reconnect(peer);
    }

    Client::Client(CppSockets::Socket &&peer, ClientConfig config) :
        socket(CppSockets::Socket(AF_INET, SOCK_STREAM, 0)), config(std::move(config))
    {
        reconnect(std::move(peer));
    }

    const ClientConfig &Client::get_config() const {
        return config;
    }

    void Client::set_config(const ClientConfig &config) {
        this->config = config;
    }

    const CppSockets::Socket &Client::get_socket() const {
        return socket;
    }

    void Client::reconnect(const CppSockets::IEndpoint &peer) {
        socket = CppSockets::Socket(AF_INET, SOCK_STREAM, 0);
        socket.connect(peer);
        // TODO authentification + protocol handshake
    }

    void Client::reconnect(CppSockets::Socket &&peer) {
        if (!peer.connected()) {
            throw std::runtime_error("Socket is not connected");
        }
        socket = std::move(peer);
        // TODO authentification + protocol handshake
    }
}
