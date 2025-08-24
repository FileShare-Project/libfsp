/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Jul 16 11:25:51 2023 Francois Michaut
** Last update Thu Aug 14 19:36:01 2025 Francois Michaut
**
** RequestData.hpp : RequestData interface. Subclasses will represent every request payload
*/

#pragma once

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Protocol/Version.hpp"
#include "FileShare/Utils/FileHash.hpp"

namespace FileShare::Protocol {
    class IRequestData {
        public:
            virtual ~IRequestData() = default;

            [[nodiscard]] virtual auto debug_str() const -> std::string = 0;
    };

    class ResponseData : public IRequestData {
        public:
            ResponseData(StatusCode status);
             ~ResponseData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;

            StatusCode status;
    };

    class SupportedVersionsData : public IRequestData {
        public:
            SupportedVersionsData(std::vector<Version> versions);
            ~SupportedVersionsData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;

            std::vector<Version> versions;
    };

    class SelectedVersionData : public IRequestData {
        public:
            SelectedVersionData(Version version);
            ~SelectedVersionData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;

            Version version;
    };

    class SendFileData : public IRequestData {
        public:
            SendFileData(
                std::string filepath, Utils::HashAlgorithm hash_algorithm, std::string filehash,
                std::filesystem::file_time_type last_updated, std::size_t packet_size,
                std::size_t total_packets
            );
             ~SendFileData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;

            std::string filepath;
            Utils::HashAlgorithm hash_algorithm;
            std::string filehash;
            std::filesystem::file_time_type last_updated;
            std::size_t packet_size;
            std::size_t total_packets;
    };

    class ReceiveFileData : public IRequestData {
        public:
            ReceiveFileData(std::string filepath, std::size_t packet_size, std::size_t packet_start);
             ~ReceiveFileData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;

            std::string filepath;
            std::size_t packet_size;
            std::size_t packet_start;
    };

    class ListFilesData : public IRequestData {
        public:
            ListFilesData(std::string folderpath);
             ~ListFilesData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;

            std::string folderpath;
    };

    class FileListData : public IRequestData {
        public:
            FileListData(std::uint8_t request_id, std::size_t packet_id, std::vector<FileInfo> files);
             ~FileListData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;

            std::uint8_t request_id;
            std::size_t packet_id;
            std::vector<FileInfo> files;
    };

    class DataPacketData : public IRequestData {
        public:
            DataPacketData(std::uint8_t request_id, std::size_t packet_id, std::string data);
             ~DataPacketData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;

            std::uint8_t request_id;
            std::size_t packet_id;
            std::string data;
    };

    class PingData : public IRequestData {
        public:
            PingData() = default;
             ~PingData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;
    };

    // TODO: Currently unused
    class ApprovalStatusData : public IRequestData {
        public:
            ApprovalStatusData(std::uint8_t request_message_id, bool status);
             ~ApprovalStatusData() override = default;

            [[nodiscard]] auto debug_str() const -> std::string override;

            std::uint8_t request_message_id;
            bool status;
    };
}
