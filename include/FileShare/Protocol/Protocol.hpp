/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Aug 25 22:59:37 2022 Francois Michaut
** Last update Fri Aug 15 13:57:44 2025 Francois Michaut
**
** Protocol.hpp : Main class to interract with the protocol
*/

#pragma once

#include "FileShare/Protocol/RequestData.hpp"
#include "FileShare/Protocol/Version.hpp"

#include <array>
#include <map>
#include <memory>
#include <vector>

namespace FileShare::Protocol {
    class IProtocolHandler {
        public:
            static constexpr char const * const MAGIC_BYTES = "FSP_";

            virtual ~IProtocolHandler() = default;

            static auto format_client_version_list() -> std::string_view;
            static auto format_server_selected_version(Version version) -> std::string;
            static auto parse_client_version(std::string_view raw_msg, Request &out) -> std::size_t;
            static auto parse_server_version(std::string_view raw_msg, Request &out) -> std::size_t;

            virtual auto format_send_file(std::uint8_t message_id, const SendFileData &data) -> std::string = 0;
            virtual auto format_receive_file(std::uint8_t message_id, const ReceiveFileData &data) -> std::string = 0;
            virtual auto format_list_files(std::uint8_t message_id, const ListFilesData &data) -> std::string = 0;

            virtual auto format_file_list(std::uint8_t message_id, const FileListData &data) -> std::string = 0;
            virtual auto format_data_packet(std::uint8_t message_id, const DataPacketData &data) -> std::string = 0;
            virtual auto format_ping(std::uint8_t message_id, const PingData &data) -> std::string = 0;

            virtual auto format_response(std::uint8_t message_id, const ResponseData &data) -> std::string = 0;

            virtual auto format_request(const Request &request) -> std::string = 0;
            virtual auto parse_request(std::string_view raw_msg, Request &out) -> std::size_t = 0;
    };

    class Protocol {
        public:
            Protocol(Version version);
            Protocol(Version::VersionEnum version);

            // TODO: Assignment Operator overloaded for Version. Calls set_version()

            void set_version(Version version);
            [[nodiscard]] auto version() const -> Version { return m_version; }

            auto operator==(const Protocol &) const -> bool = default;
            auto operator<=>(const Protocol&) const = default;

            [[nodiscard]] auto handler() const -> IProtocolHandler & { return *m_handler; }

            const static std::map<Version, std::shared_ptr<IProtocolHandler>> PROTOCOL_LIST;
        private:
            Version m_version;
            std::shared_ptr<IProtocolHandler> m_handler;
    };
}
