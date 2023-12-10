/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Jul 16 11:25:51 2023 Francois Michaut
** Last update Sat Dec  9 18:54:32 2023 Francois Michaut
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
            virtual ~ResponseData() = default;

            std::string debug_str() const override;
            StatusCode status;
    };

    class SendFileData : public IRequestData {
        public:
            SendFileData() = default;
            SendFileData(std::string filepath, Utils::HashAlgorithm hash_algorithm, std::string filehash, std::filesystem::file_time_type last_updated, std::size_t packet_size, std::size_t total_packets);
            virtual ~SendFileData() = default;

            std::string debug_str() const override;

            std::string filepath;
            Utils::HashAlgorithm hash_algorithm;
            std::string filehash;
            std::filesystem::file_time_type last_updated;
            std::size_t packet_size;
            std::size_t total_packets;
    };

    class ReceiveFileData : public IRequestData {
        public:
            ReceiveFileData() = default;
            ReceiveFileData(std::string filepath, std::size_t packet_size, std::size_t packet_start);
            virtual ~ReceiveFileData() = default;

            std::string debug_str() const override;

            std::string filepath;
            std::size_t packet_size;
            std::size_t packet_start;
    };

    class ListFilesData : public IRequestData {
        public:
            ListFilesData(std::string folderpath = "");
            virtual ~ListFilesData() = default;

            std::string debug_str() const override;

            std::string folderpath;
    };

    class FileListData : public IRequestData {
        public:
            FileListData() = default;
            FileListData(std::uint8_t request_id, std::size_t packet_id, std::vector<FileInfo> files = {});
            virtual ~FileListData() = default;

            std::string debug_str() const override;

            std::uint8_t request_id;
            std::size_t packet_id;
            std::vector<FileInfo> files;
    };

    class DataPacketData : public IRequestData {
        public:
            DataPacketData() = default;
            DataPacketData(std::uint8_t request_id, std::size_t packet_id, std::string data);
            virtual ~DataPacketData() = default;

            std::string debug_str() const override;

            std::uint8_t request_id;
            std::size_t packet_id;
            std::string data;
    };

    class PingData : public IRequestData {
        public:
            PingData() = default;
            virtual ~PingData() = default;

            std::string debug_str() const override;
    };

    class ApprovalStatusData : public IRequestData {
        public:
            ApprovalStatusData() = default;
            ApprovalStatusData(std::uint8_t request_message_id, bool status);
            virtual ~ApprovalStatusData() = default;

            std::string debug_str() const override;

            std::uint8_t request_message_id;
            bool status;
    };
}
