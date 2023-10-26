/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue Jul 18 22:04:57 2023 Francois Michaut
** Last update Sun Oct 22 13:16:24 2023 Francois Michaut
**
** RequestData.cpp : RequestData implementation for the requests payloads
*/

#include "FileShare/Protocol/RequestData.hpp"
#include "FileShare/Utils/Serialize.hpp"
#include "FileShare/Utils/Time.hpp"

static std::string debug_str(FileShare::Protocol::Page page) {
    std::stringstream ss;

    ss << "Page{"
       << "total = " << page.total
       << ", current = " << page.current
       << "}";
    return ss.str();
}

namespace FileShare::Protocol {
    ResponseData::ResponseData(StatusCode status) :
        status(status)
    {}

    SendFileData::SendFileData(std::string filepath, Utils::HashAlgorithm hash_algorithm, std::string filehash, std::filesystem::file_time_type last_updated, std::size_t packet_size, std::size_t total_packets) :
        filepath(filepath), hash_algorithm(hash_algorithm), filehash(filehash), last_updated(last_updated), packet_size(packet_size), total_packets(total_packets)
    {}

    ReceiveFileData::ReceiveFileData(std::string filepath, std::size_t packet_size, std::size_t packet_start) :
        filepath(filepath), packet_size(packet_size), packet_start(packet_start)
    {}

    ListFilesData::ListFilesData(std::string folderpath, std::size_t page_nb) :
        folderpath(folderpath), page_nb(page_nb)
    {}

    FileListData::FileListData(std::vector<FileInfo> files, Page page) :
        files(files), page(page)
    {}

    DataPacketData::DataPacketData(std::uint8_t request_id, std::size_t packet_id, std::string data) :
        request_id(request_id), packet_id(packet_id), data(data)
    {}

    ApprovalStatusData::ApprovalStatusData(std::uint8_t request_message_id, bool status) :
        request_message_id(request_message_id), status(status)
    {}

    std::string ResponseData::debug_str() const {
        std::stringstream ss;

        ss << "ResponseData{"
           << "status = " << status
           << "}";
        return ss.str();
    }

    std::string PingData::debug_str() const {
        std::stringstream ss;

        ss << "PingData{}";
        return ss.str();
    }

    std::string DataPacketData::debug_str() const {
        std::stringstream ss;

        ss << "DataPacketData{"
           << "request_id = " << (int)request_id
           << ", packed_id = " << packet_id
           << ", data_size = " << data.size()
           << "}";
        return ss.str();
    }

    std::string FileListData::debug_str() const {
        std::stringstream ss;

        ss << "FileListData{"
           << "page = " << ::debug_str(page)
           << ", files (count = " << files.size() << ") = [";
        for (const auto &file : files) {
            ss << "{ path = " << file.path << ", file_type = " << (int)file.file_type << " }, ";
        }
        ss   << "]}";
        return ss.str();
    }

    std::string ListFilesData::debug_str() const {
        std::stringstream ss;

        ss << "ListFilesData{"
           << "folderpath = " << folderpath
           << ", page_nb = " << page_nb
           << "}";
        return ss.str();
    }

    std::string ReceiveFileData::debug_str() const {
        std::stringstream ss;

        ss << "ReceiveFileData{"
           << "filepath = " << filepath
           << ", packet_size = " << packet_size
           << ", packet_start = " << packet_start
           << "}";
        return ss.str();
    }

    std::string SendFileData::debug_str() const {
        std::stringstream ss;

        ss << "SendFileData{"
           << "filepath = " << filepath
           << ", hash_algo = " << Utils::algo_to_string(hash_algorithm)
           << ", file_hash = " << filehash
           << ", last_updated = " << Utils::to_epoch(last_updated)
           << ", packet_size = " << packet_size
           << ", total_packets = " << total_packets
           << "}";
        return ss.str();
    }

    std::string ApprovalStatusData::debug_str() const {
        std::stringstream ss;

        ss << "ApprovalStatusData{"
           << "request_message_id = " << (int)request_message_id
           << ", status = " << status
           << "}";
        return ss.str();
    }
}
