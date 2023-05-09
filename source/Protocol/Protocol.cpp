/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Aug 25 23:16:42 2022 Francois Michaut
** Last update Tue May  9 09:12:49 2023 Francois Michaut
**
** Protocol.cpp : Implementation of the main Protocol class
*/

#include "FileShare/Protocol/Protocol.hpp"

namespace FileShare::Protocol {
    std::map<Version, std::shared_ptr<IProtocolHandler>> Protocol::protocol_list = {
    };

    Protocol::Protocol(Version version) :
        m_version(version)
    {
        set_version(version);
    }

    void Protocol::set_version(Version version) {
        auto iter = protocol_list.find(version);

        if (iter == protocol_list.end()) {
            throw std::runtime_error("Unsuported protocol version");
        }
        m_handler = iter->second;
    }
}
