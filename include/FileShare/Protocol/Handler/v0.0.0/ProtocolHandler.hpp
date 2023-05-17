/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Fri May  5 21:32:03 2023 Francois Michaut
** Last update Mon May 15 11:52:31 2023 Francois Michaut
**
** ProtocolHandler.hpp : ProtocolHandler for the v0.0.0 of the protocol
*/

#pragma once

#include "FileShare/Protocol/Protocol.hpp"

namespace FileShare::Protocol::Handler::v0_0_0 {
    class ProtocolHandler : public IProtocolHandler {
        public:
            std::string format_send_file(std::uint8_t message_id, std::string filepath, Utils::HashAlgorithm algo) override;
            std::string format_receive_file(std::uint8_t message_id, std::string filepath, std::size_t packet_size, std::size_t packet_start) override;
            std::string format_list_files(std::uint8_t message_id, std::string folderpath = "", std::size_t page_size = 0, std::size_t page_idx = 0) override;

            std::string format_file_list(std::uint8_t message_id, std::vector<FileInfo> files, std::size_t page_idx, std::size_t total_pages) override;
            std::string format_data_packet(std::uint8_t message_id, std::string filepath, std::size_t packet_idx, std::string_view data) override;
            std::string format_ping(std::uint8_t message_id) override;
        private:
            const static std::size_t packet_size = 4096; // TODO experiment with this
    };
}
