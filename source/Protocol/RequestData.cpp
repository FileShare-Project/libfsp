/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue Jul 18 22:04:57 2023 Francois Michaut
** Last update Tue Jul 18 22:13:10 2023 Francois Michaut
**
** RequestData.cpp : RequestData implementation for the requests payloads
*/

#include "FileShare/Protocol/RequestData.hpp"

namespace FileShare::Protocol {
    IRequestData::IRequestData(std::uint8_t message_id) :
        message_id(message_id)
    {}

    PingData::PingData(std::uint8_t message_id) :
        IRequestData(message_id)
    {}

    DataPacketData::DataPacketData(std::uint8_t message_id) :
        IRequestData(message_id)
    {}

    FileListData::FileListData(std::uint8_t message_id) :
        IRequestData(message_id)
    {}

    ListFilesData::ListFilesData(std::uint8_t message_id) :
        IRequestData(message_id)
    {}

    ReceiveFileData::ReceiveFileData(std::uint8_t message_id) :
        IRequestData(message_id)
    {}

    SendFileData::SendFileData(std::uint8_t message_id) :
        IRequestData(message_id)
    {}
}
