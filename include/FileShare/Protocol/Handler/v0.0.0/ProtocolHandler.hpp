/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Fri May  5 21:32:03 2023 Francois Michaut
** Last update Sat Dec  9 08:59:02 2023 Francois Michaut
**
** ProtocolHandler.hpp : ProtocolHandler for the v0.0.0 of the protocol
*/

#pragma once

#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/Protocol/RequestData.hpp"

namespace FileShare::Protocol::Handler::v0_0_0 {
    class ProtocolHandler : public IProtocolHandler {
        public:
            virtual ~ProtocolHandler() = default;

            std::string format_send_file(std::uint8_t message_id, const SendFileData &data) override;
            std::string format_receive_file(std::uint8_t message_id, const ReceiveFileData &data) override;
            std::string format_list_files(std::uint8_t message_id, const ListFilesData &data) override;

            std::string format_file_list(std::uint8_t message_id, const FileListData &data) override;
            std::string format_data_packet(std::uint8_t message_id, const DataPacketData &data) override;
            std::string format_ping(std::uint8_t message_id, const PingData &data) override;

            std::string format_response(std::uint8_t message_id, const ResponseData &data) override;

            std::string format_request(const Request &request) override;
            std::size_t parse_request(std::string_view raw_msg, Request &out) override;
        private:
            const static std::size_t packet_size = 4096; // TODO experiment with this
            const static std::size_t base_header_size = 6;

            std::shared_ptr<IRequestData> get_request_data(CommandCode cmd, std::string_view payload);

            std::shared_ptr<IRequestData> parse_response(std::string_view payload);
            std::shared_ptr<IRequestData> parse_send_file(std::string_view payload);
            std::shared_ptr<IRequestData> parse_receive_file(std::string_view payload);
            std::shared_ptr<IRequestData> parse_list_files(std::string_view payload);
            std::shared_ptr<IRequestData> parse_file_list(std::string_view payload);
            std::shared_ptr<IRequestData> parse_data_packet(std::string_view payload);
            std::shared_ptr<IRequestData> parse_ping(std::string_view payload);
    };
}
