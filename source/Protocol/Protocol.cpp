/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Aug 25 23:16:42 2022 Francois Michaut
** Last update Fri Aug 22 23:17:35 2025 Francois Michaut
**
** Protocol.cpp : Implementation of the main Protocol class
*/

#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Protocol/Handler/v0.0.0/ProtocolHandler.hpp"
#include "FileShare/Utils/Strings.hpp"

#include <string_view>
#include <unordered_map>

namespace FileShare::Protocol {
    const std::map<Version, std::shared_ptr<IProtocolHandler>> Protocol::PROTOCOL_LIST = {
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
        auto iter = PROTOCOL_LIST.find(version);

        if (iter == PROTOCOL_LIST.end()) {
            throw std::runtime_error("Unsuported protocol version");
        }
        m_handler = iter->second;
    }

    auto str_to_command(std::string_view str) -> CommandCode {
        const static std::unordered_map<std::string, CommandCode, FileShare::Utils::string_hash, std::equal_to<>> str_to_command = {
            {"RESPONSE", CommandCode::RESPONSE},

            {"SEND_FILE", CommandCode::SEND_FILE},
            {"RECEIVE_FILE", CommandCode::RECEIVE_FILE},

            {"LIST_FILES", CommandCode::LIST_FILES},
            {"FILE_LIST", CommandCode::FILE_LIST},

            {"PING", CommandCode::PING},
            {"DATA_PACKET", CommandCode::DATA_PACKET},

            {"PAIR_REQUEST", CommandCode::PAIR_REQUEST},
            {"ACCEPT_PAIR_REQUEST", CommandCode::ACCEPT_PAIR_REQUEST},
        };

        auto iter = str_to_command.find(str);

        if (iter != str_to_command.end()) {
            return iter->second;
        }
        throw std::runtime_error("Unknown command");
    }

    auto str_to_status(std::string_view str) -> StatusCode {
        const static std::unordered_map<std::string, StatusCode, FileShare::Utils::string_hash, std::equal_to<>> str_to_status = {
            {"STATUS_OK", StatusCode::STATUS_OK},
            {"UP_TO_DATE", StatusCode::UP_TO_DATE},
            {"MESSAGE_TOO_LONG", StatusCode::MESSAGE_TOO_LONG},
            {"APPROVAL_PENDING", StatusCode::APPROVAL_PENDING},

            {"BAD_REQUEST", StatusCode::BAD_REQUEST},
            {"INVALID_REQUEST_ID", StatusCode::INVALID_REQUEST_ID},
            {"FORBIDDEN", StatusCode::FORBIDDEN},
            {"FILE_NOT_FOUND", StatusCode::FILE_NOT_FOUND},
            {"UNKNOWN_COMMAND", StatusCode::UNKNOWN_COMMAND},
            {"TOO_MANY_REQUESTS", StatusCode::TOO_MANY_REQUESTS},

            {"INTERNAL_ERROR", StatusCode::INTERNAL_ERROR},
        };
        auto iter = str_to_status.find(str);

        if (iter != str_to_status.end()) {
            return iter->second;
        }
        throw std::runtime_error("Unknown status");
    }

    auto str_to_file_type(std::string_view str) -> FileType {
        const static std::unordered_map<std::string, FileType, FileShare::Utils::string_hash, std::equal_to<>> str_to_file_type = {
            {"FILE", FileType::FILE},
            {"DIRECTORY", FileType::DIRECTORY},
        };

        auto iter = str_to_file_type.find(str);

        if (iter != str_to_file_type.end()) {
            return iter->second;
        }
        throw std::runtime_error("Unknown FileType");
    }

    auto command_to_str(CommandCode command) -> std::string_view {
        const static std::unordered_map<CommandCode, const char *> command_to_str = {
            {CommandCode::RESPONSE, "RESPONSE"},

            {CommandCode::SEND_FILE, "SEND_FILE"},
            {CommandCode::RECEIVE_FILE, "RECEIVE_FILE"},

            {CommandCode::LIST_FILES, "LIST_FILES"},
            {CommandCode::FILE_LIST, "FILE_LIST"},

            {CommandCode::PING, "PING"},
            {CommandCode::DATA_PACKET, "DATA_PACKET"},

            {CommandCode::PAIR_REQUEST, "PAIR_REQUEST"},
            {CommandCode::ACCEPT_PAIR_REQUEST, "ACCEPT_PAIR_REQUEST"},
        };

        auto iter = command_to_str.find(command);

        if (iter != command_to_str.end()) {
            return iter->second;
        }

        return "__UNKNOWN_COMMAND__";
    }

    auto status_to_str(StatusCode status) -> std::string_view {
        const static std::unordered_map<StatusCode, const char *> status_to_str = {
            {StatusCode::STATUS_OK, "STATUS_OK"},
            {StatusCode::UP_TO_DATE, "UP_TO_DATE"},
            {StatusCode::MESSAGE_TOO_LONG, "MESSAGE_TOO_LONG"},
            {StatusCode::APPROVAL_PENDING, "APPROVAL_PENDING"},

            {StatusCode::BAD_REQUEST, "BAD_REQUEST"},
            {StatusCode::UNAUTHORIZED, "UNAUTHORIZED"},
            {StatusCode::INVALID_REQUEST_ID, "INVALID_REQUEST_ID"},
            {StatusCode::FORBIDDEN, "FORBIDDEN"},
            {StatusCode::FILE_NOT_FOUND, "FILE_NOT_FOUND"},
            {StatusCode::UNKNOWN_COMMAND, "UNKNOWN_COMMAND"},
            {StatusCode::TOO_MANY_REQUESTS, "TOO_MANY_REQUESTS"},

            {StatusCode::INTERNAL_ERROR, "INTERNAL_ERROR"},
        };

        auto iter = status_to_str.find(status);

        if (iter != status_to_str.end()) {
            return iter->second;
        }

        return "__UNKNOWN_STATUS__";
    }

    auto file_type_to_str(FileType type) -> std::string_view {
        const static std::unordered_map<FileType, const char *> file_type_to_str = {
            {FileType::FILE, "FILE"},
            {FileType::DIRECTORY, "DIRECTORY"},
        };

        auto iter = file_type_to_str.find(type);

        if (iter != file_type_to_str.end()) {
            return iter->second;
        }

        return "__UNKNOWN_STATUS__";
    }
}

auto operator<<(std::ostream& os, const FileShare::Protocol::StatusCode& status) -> std::ostream & {
    return os << status_to_str(status);
}

auto operator<<(std::ostream& os, const FileShare::Protocol::CommandCode& command) -> std::ostream & {
    return os << command_to_str(command);
}

auto operator<<(std::ostream& os, const FileShare::Protocol::FileType &type) -> std::ostream & {
    return os << file_type_to_str(type);
}

auto operator>>(std::istream& is, FileShare::Protocol::StatusCode& status) -> std::istream & {
    std::string str;

    is >> str;
    status = FileShare::Protocol::str_to_status(str);
    return is;
}

auto operator>>(std::istream& is, FileShare::Protocol::CommandCode& command) -> std::istream & {
    std::string str;

    is >> str;
    command = FileShare::Protocol::str_to_command(str);
    return is;
}

auto operator>>(std::istream& is, FileShare::Protocol::FileType &type) -> std::istream & {
    std::string str;

    is >> str;
    type = FileShare::Protocol::str_to_file_type(str);
    return is;
}
