/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Aug 25 23:16:42 2022 Francois Michaut
** Last update Thu Aug 24 09:44:46 2023 Francois Michaut
**
** Protocol.cpp : Implementation of the main Protocol class
*/

#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/Protocol/Handler/v0.0.0/ProtocolHandler.hpp"

namespace FileShare::Protocol {
    std::map<Version, std::shared_ptr<IProtocolHandler>> Protocol::protocol_list = {
        {Version::v0_0_0, std::make_shared<Handler::v0_0_0::ProtocolHandler>()}
    };

    Protocol::Protocol(Version version) :
        m_version(version)
    {
        set_version(version);
    }

    Protocol::Protocol(Version::VersionEnum version) :
        Protocol::Protocol(Version(version))
    {}

    void Protocol::set_version(Version version) {
        auto iter = protocol_list.find(version);

        if (iter == protocol_list.end()) {
            throw std::runtime_error("Unsuported protocol version");
        }
        m_handler = iter->second;
    }
}

std::ostream& operator<<(std::ostream& os, const FileShare::Protocol::StatusCode& status) {
    using namespace FileShare::Protocol;

    switch (status) {
        case StatusCode::STATUS_OK:
            return os << "STATUS_OK";
        case StatusCode::MESSAGE_TOO_LONG:
            return os << "MESSAGE_TOO_LONG";
        case StatusCode::APPROVAL_PENDING:
            return os << "APPROVAL_PENDING";

        case StatusCode::BAD_REQUEST:
            return os << "BAD_REQUEST";
        case StatusCode::INVALID_REQUEST_ID:
            return os << "INVALID_REQUEST_ID";
        case StatusCode::FORBIDDEN:
            return os << "FORBIDDEN";
        case StatusCode::FILE_NOT_FOUND:
            return os << "FILE_NOT_FOUND";
        case StatusCode::UNKNOWN_COMMAND:
            return os << "UNKNOWN_COMMAND";
        case StatusCode::TOO_MANY_REQUESTS:
            return os << "TOO_MANY_REQUESTS";

        case StatusCode::INTERNAL_ERROR:
            return os << "INTERNAL_ERROR";

        default:
            return os << "__UNKNOWN_STATUS__";
    }
}

std::ostream& operator<<(std::ostream& os, const FileShare::Protocol::CommandCode& command) {
    using namespace FileShare::Protocol;

    switch (command) {
        case CommandCode::RESPONSE:
            return os << "RESPONSE";

        case CommandCode::SEND_FILE:
            return os << "SEND_FILE";
        case CommandCode::RECEIVE_FILE:
            return os << "RECEIVE_FILE";

        case CommandCode::LIST_FILES:
            return os << "LIST_FILES";
        case CommandCode::FILE_LIST:
            return os << "FILE_LIST";

        case CommandCode::PING:
            return os << "PING";
        case CommandCode::APPROVAL_STATUS:
            return os << "APPROVAL_STATUS";
        case CommandCode::DATA_PACKET:
            return os << "DATA_PACKET";

        case CommandCode::PAIR_REQUEST:
            return os << "PAIR_REQUEST";
        case CommandCode::ACCEPT_PAIR_REQUEST:
            return os << "ACCEPT_PAIR_REQUEST";
        default:
            return os << "__UNKNOWN_COMMAND__";
    }
}
