/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Fri May  5 21:32:03 2023 Francois Michaut
** Last update Fri Aug 15 13:48:49 2025 Francois Michaut
**
** ProtocolHandler.hpp : ProtocolHandler for the v0.0.0 of the protocol
*/

#pragma once

#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/Protocol/RequestData.hpp"

namespace FileShare::Protocol::Handler::v0_0_0 { // NOLINT(readability-identifier-naming)
    class ProtocolHandler : public IProtocolHandler {
        public:
            ~ProtocolHandler() override = default;

            auto format_send_file(std::uint8_t message_id, const SendFileData &data) -> std::string override;
            auto format_receive_file(std::uint8_t message_id, const ReceiveFileData &data) -> std::string override;
            auto format_list_files(std::uint8_t message_id, const ListFilesData &data) -> std::string override;

            auto format_file_list(std::uint8_t message_id, const FileListData &data) -> std::string override;
            auto format_data_packet(std::uint8_t message_id, const DataPacketData &data) -> std::string override;
            auto format_ping(std::uint8_t message_id, const PingData &data) -> std::string override;

            auto format_response(std::uint8_t message_id, const ResponseData &data) -> std::string override;

            auto format_request(const Request &request) -> std::string override;
            auto parse_request(std::string_view raw_msg, Request &out) -> std::size_t override;
        private:
            constexpr static std::size_t PACKET_SIZE = 4096; // TODO experiment with this
            constexpr static std::size_t BASE_HEADER_SIZE = 6;

            auto get_request_data(CommandCode cmd, std::string_view payload) -> std::shared_ptr<IRequestData>;

            auto parse_response(std::string_view payload) -> std::shared_ptr<IRequestData>;
            auto parse_send_file(std::string_view payload) -> std::shared_ptr<IRequestData>;
            auto parse_receive_file(std::string_view payload) -> std::shared_ptr<IRequestData>;
            auto parse_list_files(std::string_view payload) -> std::shared_ptr<IRequestData>;
            auto parse_file_list(std::string_view payload) -> std::shared_ptr<IRequestData>;
            auto parse_data_packet(std::string_view payload) -> std::shared_ptr<IRequestData>;
            auto parse_ping(std::string_view payload) -> std::shared_ptr<IRequestData>;
    };
}
