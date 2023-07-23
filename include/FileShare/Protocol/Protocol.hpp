/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Aug 25 22:59:37 2022 Francois Michaut
** Last update Thu Jul 20 21:01:14 2023 Francois Michaut
**
** Protocol.hpp : Main class to interract with the protocol
*/

#pragma once

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Protocol/Version.hpp"
#include "FileShare/Utils/FileHash.hpp"

#include <map>
#include <memory>
#include <vector>

namespace FileShare::Protocol {
    class IProtocolHandler {
        public:
            static constexpr char const * const magic_bytes = "FSP_";

            virtual std::string format_send_file(std::uint8_t message_id, std::string filepath, Utils::HashAlgorithm algo) = 0;
            // packet_size has no effect without packet_start: it is only there so the sender can do packet_size * packet_start and compute the start byte
            virtual std::string format_receive_file(std::uint8_t message_id, std::string filepath, std::size_t packet_size = 0, std::size_t packet_start = 0) = 0;
            virtual std::string format_list_files(std::uint8_t message_id, std::string folderpath = "", std::size_t page_size = 0, std::size_t page_idx = 0) = 0;

            virtual std::string format_file_list(std::uint8_t message_id, std::vector<FileInfo> files, std::size_t page_idx, std::size_t total_pages) = 0;
            virtual std::string format_data_packet(std::uint8_t message_id, std::string filepath, std::size_t packet_idx, std::string_view data) = 0;
            virtual std::string format_ping(std::uint8_t message_id) = 0;

            virtual std::size_t parse_request(std::string_view raw_msg, Request &out) = 0;
    };

    class Protocol {
        public:
            Protocol(Version version);
            Protocol(Version::VersionEnum version);

            void set_version(Version version);
            Version version() const { return m_version; }

            bool operator==(const Protocol &) const = default;
            auto operator<=>(const Protocol&) const = default;

            IProtocolHandler &handler() const { return *m_handler; }

            static std::map<Version, std::shared_ptr<IProtocolHandler>> protocol_list;
        private:
            Version m_version;
            std::shared_ptr<IProtocolHandler> m_handler;
    };
}
