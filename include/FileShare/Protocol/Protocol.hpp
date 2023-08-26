/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Aug 25 22:59:37 2022 Francois Michaut
** Last update Thu Aug 24 09:41:27 2023 Francois Michaut
**
** Protocol.hpp : Main class to interract with the protocol
*/

#pragma once

#include "FileShare/Protocol/RequestData.hpp"
#include "FileShare/Protocol/Version.hpp"
#include "FileShare/Utils/FileHash.hpp"

#include <map>
#include <memory>
#include <vector>

namespace FileShare::Protocol {
    class IProtocolHandler {
        public:
            static constexpr char const * const magic_bytes = "FSP_";

            virtual std::string format_send_file(std::uint8_t message_id, const SendFileData &data) = 0;
            virtual std::string format_receive_file(std::uint8_t message_id, const ReceiveFileData &data) = 0;
            virtual std::string format_list_files(std::uint8_t message_id, const ListFilesData &data) = 0;

            virtual std::string format_file_list(std::uint8_t message_id, const FileListData &data) = 0;
            virtual std::string format_data_packet(std::uint8_t message_id, const DataPacketData &data) = 0;
            virtual std::string format_ping(std::uint8_t message_id, const PingData &data) = 0;

            virtual std::string format_response(std::uint8_t message_id, const ResponseData &data) = 0;

            virtual std::string format_request(const Request &request) = 0;
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
