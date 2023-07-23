/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue Jul 18 22:04:57 2023 Francois Michaut
** Last update Sat Jul 22 20:41:40 2023 Francois Michaut
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

    std::string PingData::to_str() const {
        return "PingData{}";
    }

    std::string DataPacketData::to_str() const {
        std::stringstream ss;

        ss << "DataPacketData{filepath = " << filepath
           << ", packed_id = " << packet_id
           << ", packed_size = " << packet_size
           << ", data = " << data
           << "}";
        return ss.str();
    }

    std::string FileListData::to_str() const {
        std::stringstream ss;

        ss << "FileListData{total_pages = " << total_pages
           << ", current_page = " << current_page
           << ", files (count = " << files.size() << ") = [";
        for (const auto &file : files) {
            ss << "{ path = " << file.path << ", file_type = " << (int)file.file_type << " }, ";
        }
        ss   << "]}";
        return ss.str();
    }

    std::string ListFilesData::to_str() const {
        std::stringstream ss;

        ss << "ListFilesData{folderpath = " << folderpath
           << ", page_nb = " << page_nb
           << ", page_size = " << page_size
           << "}";
        return ss.str();
    }

    std::string ReceiveFileData::to_str() const {
        std::stringstream ss;

        ss << "ReceiveFileData{filepath = " << filepath
           << ", packet_size = " << packet_size
           << ", packet_start = " << packet_start
           << "}";
        return ss.str();
    }

    std::string SendFileData::to_str() const {
        std::stringstream ss;

        ss << "SendFileData{filepath = " << filepath
           << ", hash_algo = " << Utils::algo_to_string(hash_algorithm)
           << ", file_hash = " << filehash
           // << ", last_updated = " << last_updated // TODO
           << ", total_packets = " << total_packets
           << "}";
        return ss.str();
    }
}
