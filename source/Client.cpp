/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Mon Sep  5 07:34:21 2022 Francois Michaut
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

    Client::Client(const CppSockets::IEndpoint &peer) :
        Client(make_socket_from_endpoint(peer))
    {}

    Client::Client(CppSockets::Socket &&peer) :
        socket(CppSockets::Socket(AF_INET, SOCK_STREAM, 0))
    {
        reconnect(std::move(peer));
    }

    void Client::reconnect(const CppSockets::IEndpoint &peer) {
        reconnect(make_socket_from_endpoint(peer));
    }

    void Client::reconnect(CppSockets::Socket &&peer) {
        if (!peer.isConnected()) {
            throw std::runtime_error("Socket is not connected");
        }
        socket = std::move(peer);
    }
}
