/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Jul 16 11:25:51 2023 Francois Michaut
** Last update Wed Dec  6 05:13:05 2023 Francois Michaut
**
** RequestData.hpp : RequestData interface. Subclasses will represent every request payload
*/

#pragma once

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Utils/FileHash.hpp"

namespace FileShare::Protocol {
    class IRequestData {
        public:
            virtual std::string debug_str() const = 0;
    };

    class ResponseData : public IRequestData {
        public:
            ResponseData() = default;
            ResponseData(StatusCode status);

            std::string debug_str() const override;
            StatusCode status;
    };

    class SendFileData : public IRequestData {
        public:
            SendFileData() = default;
            SendFileData(std::string filepath, Utils::HashAlgorithm hash_algorithm, std::string filehash, std::filesystem::file_time_type last_updated, std::size_t packet_size, std::size_t total_packets);

            std::string debug_str() const override;

            std::string filepath;
            Utils::HashAlgorithm hash_algorithm;
            std::string filehash;
            std::filesystem::file_time_type last_updated;
            std::size_t total_packets;
            std::size_t packet_size;
    };

    class ReceiveFileData : public IRequestData {
        public:
            ReceiveFileData() = default;
            ReceiveFileData(std::string filepath, std::size_t packet_size, std::size_t packet_start);

            std::string debug_str() const override;

            std::string filepath;
            std::size_t packet_size;
            std::size_t packet_start;
    };

    class ListFilesData : public IRequestData {
        public:
            ListFilesData(std::string folderpath = "");

            std::string debug_str() const override;

            std::string folderpath;
    };

    class FileListData : public IRequestData {
        public:
            FileListData() = default;
            FileListData(std::uint8_t request_id, std::size_t packet_id, std::vector<FileInfo> files = {});

            std::string debug_str() const override;

            std::uint8_t request_id;
            std::size_t packet_id;
            std::vector<FileInfo> files;
    };

    class DataPacketData : public IRequestData {
        public:
            DataPacketData() = default;
            DataPacketData(std::uint8_t request_id, std::size_t packet_id, std::string data);

            std::string debug_str() const override;

            std::uint8_t request_id;
            std::size_t packet_id;
            std::string data;
    };

    class PingData : public IRequestData {
        public:
            PingData() = default;

            std::string debug_str() const override;
    };

    class ApprovalStatusData : public IRequestData {
        public:
            ApprovalStatusData() = default;
            ApprovalStatusData(std::uint8_t request_message_id, bool status);

            std::string debug_str() const override;

            std::uint8_t request_message_id;
            bool status;
    };
}
