/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue Jul 18 22:04:57 2023 Francois Michaut
** Last update Fri Aug 15 21:04:29 2025 Francois Michaut
**
** RequestData.cpp : RequestData implementation for the requests payloads
*/

#include "FileShare/Protocol/RequestData.hpp"
#include "FileShare/Protocol/Version.hpp"
#include "FileShare/Utils/Time.hpp"

#include <sstream>
#include <utility>

namespace FileShare::Protocol {
    ResponseData::ResponseData(StatusCode status) :
        status(status)
    {}

    SupportedVersionsData::SupportedVersionsData(std::vector<Version> versions) :
        versions(std::move(versions))
    {}

    SelectedVersionData::SelectedVersionData(Version version) :
        version(version)
    {}

    SendFileData::SendFileData(
        std::string filepath, Utils::HashAlgorithm hash_algorithm, std::string filehash,
        std::filesystem::file_time_type last_updated, std::size_t packet_size, std::size_t total_packets
    ) :
        filepath(std::move(filepath)), hash_algorithm(hash_algorithm), filehash(std::move(filehash)),
        last_updated(last_updated), packet_size(packet_size), total_packets(total_packets)
    {}

    ReceiveFileData::ReceiveFileData(std::string filepath, std::size_t packet_size, std::size_t packet_start) :
        filepath(std::move(filepath)), packet_size(packet_size), packet_start(packet_start)
    {}

    ListFilesData::ListFilesData(std::string folderpath) :
        folderpath(std::move(folderpath))
    {}

    FileListData::FileListData(std::uint8_t request_id, std::size_t packet_id, std::vector<FileInfo> files) :
        request_id(request_id), packet_id(packet_id), files(std::move(files))
    {}

    DataPacketData::DataPacketData(std::uint8_t request_id, std::size_t packet_id, std::string data) :
        request_id(request_id), packet_id(packet_id), data(std::move(data))
    {}

    ApprovalStatusData::ApprovalStatusData(std::uint8_t request_message_id, bool status) :
        request_message_id(request_message_id), status(status)
    {}

    auto ResponseData::debug_str() const -> std::string {
        std::stringstream ss;

        ss << "ResponseData{"
           << "status = " << status
           << "}";
        return ss.str();
    }

    auto SupportedVersionsData::debug_str() const -> std::string {
        std::stringstream ss;

        ss << "SupportedVersionsData{"
           << "versions (count = " << versions.size() << ") = [";
        for (const auto &version : versions) {
            ss << version << ", ";
        }
        ss << "]}";
        return ss.str();
    }

    auto SelectedVersionData::debug_str() const -> std::string {
        std::stringstream ss;

        ss << "SelectedVersionData{"
           << "version = " << version
           << "}";
        return ss.str();
    }

    auto DataPacketData::debug_str() const -> std::string {
        std::stringstream ss;

        ss << "DataPacketData{"
           << "request_id = " << static_cast<int>(request_id)
           << ", packed_id = " << packet_id
           << ", data_size = " << data.size()
           << "}";
        return ss.str();
    }

    auto FileListData::debug_str() const -> std::string {
        std::stringstream ss;

        ss << "FileListData{"
           << "request_id = " << static_cast<int>(request_id)
           << ", packet_id = " << packet_id
           << ", files (count = " << files.size() << ") = [";
        for (const auto &file : files) {
            ss << "{ path = " << file.path << ", file_type = " << static_cast<int>(file.file_type) << " }, ";
        }
        ss   << "]}";
        return ss.str();
    }

    auto ListFilesData::debug_str() const -> std::string {
        std::stringstream ss;

        ss << "ListFilesData{"
           << "folderpath = " << folderpath
           << "}";
        return ss.str();
    }

    auto ReceiveFileData::debug_str() const -> std::string {
        std::stringstream ss;

        ss << "ReceiveFileData{"
           << "filepath = " << filepath
           << ", packet_size = " << packet_size
           << ", packet_start = " << packet_start
           << "}";
        return ss.str();
    }

    auto SendFileData::debug_str() const -> std::string {
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

    auto PingData::debug_str() const -> std::string {
        std::stringstream ss;

        ss << "PingData{}";
        return ss.str();
    }

    auto ApprovalStatusData::debug_str() const -> std::string {
        std::stringstream ss;

        ss << "ApprovalStatusData{"
           << "request_message_id = " << static_cast<int>(request_message_id)
           << ", status = " << status
           << "}";
        return ss.str();
    }
}
