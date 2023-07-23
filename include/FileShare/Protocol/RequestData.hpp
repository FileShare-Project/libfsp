/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Jul 16 11:25:51 2023 Francois Michaut
** Last update Sat Jul 22 20:44:28 2023 Francois Michaut
**
** RequestData.hpp : RequestData interface. Subclasses will represent every request payload
*/

#pragma once

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Utils/FileHash.hpp"

namespace FileShare::Protocol {
    class IRequestData {
        public:
            IRequestData(std::uint8_t message_id);

            virtual std::string to_str() const = 0;
            std::uint8_t message_id;
    };

    class SendFileData : public IRequestData {
        public:
            SendFileData(std::uint8_t message_id);

            std::string to_str() const override;

            std::string filepath;
            Utils::HashAlgorithm hash_algorithm;
            std::string filehash;
            std::filesystem::file_time_type last_updated;
            std::size_t total_packets;
    };

    class ReceiveFileData : public IRequestData {
        public:
            ReceiveFileData(std::uint8_t message_id);

            std::string to_str() const override;

            std::string filepath;
            std::size_t packet_size;
            std::size_t packet_start;
    };

    class ListFilesData : public IRequestData {
        public:
            ListFilesData(std::uint8_t message_id);

            std::string to_str() const override;

            std::string folderpath;
            std::size_t page_nb;
            std::size_t page_size;
    };

    class FileListData : public IRequestData {
        public:
            FileListData(std::uint8_t message_id);

            std::string to_str() const override;

            std::size_t total_pages;
            std::size_t current_page;
            std::vector<FileInfo> files;
    };

    class DataPacketData : public IRequestData {
        public:
            DataPacketData(std::uint8_t message_id);

            std::string to_str() const override;

            std::string filepath;
            std::size_t packet_id;
            std::size_t packet_size;
            std::string data;
    };

    class PingData : public IRequestData {
        public:
            PingData(std::uint8_t message_id);

            std::string to_str() const override;
    };
}
