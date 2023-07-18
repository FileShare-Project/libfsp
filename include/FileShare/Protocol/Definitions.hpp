/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:28:47 2022 Francois Michaut
** Last update Tue Jul 18 08:55:17 2023 Francois Michaut
**
** Definitions.hpp : General definitions and classes
*/

#pragma once

#include <memory>
#include <string>
#include <vector>

// TODO: find a better header name / organisation

namespace FileShare::Protocol {
    // TODO: rename MessageCode or similar (it is both Requests and Responses)

    // 1 byte - 0-0xFF
    enum class CommandCode {
        SEND_FILE           = 0x10,
        RECIVE_FILE         = 0x11,

        LIST_FILES          = 0x20,
        FILE_LIST           = 0x21,

        PING                = 0x30,
        DATA_PACKET         = 0x42,

        PAIR_REQUEST        = 0x50,
        ACCEPT_PAIR_REQUEST = 0x51,
    };

    // 1 byte - 0-0xFF
    enum class StatusCode {
        STATUS_OK           = 0x00,

        MESSAGE_TOO_LONG    = 0x24,

        APPROVAL_PENDING    = 0x33, // SEND_FILE / RECEIVE_FILE needing user confirmation
                                    // TODO: figure out next step on approval/reject ?

        BAD_REQUEST         = 0x40,
        INVALID_PATH        = 0x42,
        FORBIDDEN           = 0x43,
        FILE_NOT_FOUND      = 0x44,
        UNKNOWN_COMMAND     = 0x45,
        TOO_MANY_REQUESTS   = 0x49,

        INTERNAL_ERROR      = 0x50,
    };

    enum class FileType {
        FILE       = 0x00,
        DIRECTORY  = 0x01,
    };

    class IRequestData;
    struct Request {
        CommandCode code;
        std::shared_ptr<IRequestData> request;
    };

    template<class T>
    struct Response {
        StatusCode code;
        std::shared_ptr<T> response;
    };

    struct FileInfo {
        std::string path;
        FileType file_type;
    };

    struct FileList { // TODO: remove
        std::vector<FileInfo> files;
        std::size_t page_nb;
        std::size_t total_pages;
    };
}
