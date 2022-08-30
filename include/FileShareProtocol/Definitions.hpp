/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:28:47 2022 Francois Michaut
** Last update Mon Aug 29 18:56:09 2022 Francois Michaut
**
** Definitions.hpp : General definitions and classes
*/

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
        FORBIDDEN       = 0x43,
        UNKNOWN_COMMAND = 0x44,
        INTERNAL_ERROR  = 0x50,
    };

    class Response {
        private:
            StatusCode code;

    };
}
