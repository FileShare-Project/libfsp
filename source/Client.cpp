/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Mon Aug 29 21:14:48 2022 Francois Michaut
**
** Client.cpp : Implementation of the FileShareProtocol Client
*/

#include "FileShareProtocol/Client.hpp"

namespace FileShareProtocol {
    Client::Client(const CppSockets::IEndpoint &peer) :
        Client(CppSockets::Socket(AF_INET, SOCK_STREAM, 0))
    {
        socket.connect(peer);
    }

    Client::Client(CppSockets::Socket &&peer) :
        socket(std::move(peer))
    {}
}
