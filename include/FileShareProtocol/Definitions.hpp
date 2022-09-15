/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:28:47 2022 Francois Michaut
** Last update Wed Sep 14 22:27:04 2022 Francois Michaut
**
** Definitions.hpp : General definitions and classes
*/

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace FileShareProtocol {
    enum class CommandCode {
        REQ_SEND_FILE       = 0x10,
        REQ_RECIVE_FILE     = 0x11,
        SEND_FILE           = 0x20,
        RECIVE_FILE         = 0x21,
        LIST_FILES          = 0x30,
        FILE_LIST           = 0x31,
        DATA_PACKET         = 0x42,
        PAIR_REQUEST        = 0x50,
        ACCEPT_PAIR_REQUEST = 0x51,
    };

    enum class StatusCode {
        STATUS_OK       = 0x00,
        INVALID_PATH    = 0x42,
        FORBIDDEN       = 0x43,
        FILE_NOT_FOUND  = 0x44,
        UNKNOWN_COMMAND = 0x45,
        INTERNAL_ERROR  = 0x50,
    };

    template<class T>
    struct Response {
        StatusCode code;
        std::shared_ptr<T> response;
    };

    struct FileInfo {
        std::string path;
        bool is_directory;
    };

    struct FileList {
        std::vector<FileInfo> files;
        std::size_t page_nb;
        std::size_t total_pages;
    };
}
