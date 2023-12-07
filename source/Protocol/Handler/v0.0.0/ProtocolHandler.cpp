/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Fri May  5 21:35:06 2023 Francois Michaut
** Last update Mon Dec  4 19:18:12 2023 Francois Michaut
**
** ProtocolHandler.cpp : ProtocolHandler for the v0.0.0 of the protocol
*/

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Protocol/Handler/v0.0.0/ProtocolHandler.hpp"
#include "FileShare/Protocol/RequestData.hpp"
#include "FileShare/Utils/Serialize.hpp"
#include "FileShare/Utils/Time.hpp"
#include "FileShare/Utils/VarInt.hpp"

#include <chrono>

// TODO: enfore 8bytes limits on VARINTs

namespace FileShare::Protocol::Handler::v0_0_0 {
    std::string ProtocolHandler::format_request(const Request &request) {
        switch (request.code) {
            case CommandCode::RESPONSE: {
                auto data = std::dynamic_pointer_cast<ResponseData>(request.request);

                return format_response(request.message_id, *data);
            }
            case CommandCode::SEND_FILE: {
                auto data = std::dynamic_pointer_cast<SendFileData>(request.request);

                return format_send_file(request.message_id, *data);
            }
            case CommandCode::RECEIVE_FILE: {
                auto data = std::dynamic_pointer_cast<ReceiveFileData>(request.request);

                return format_receive_file(request.message_id, *data);
            }
            case CommandCode::LIST_FILES: {
                auto data = std::dynamic_pointer_cast<ListFilesData>(request.request);

                return format_list_files(request.message_id, *data);
            }
            case CommandCode::FILE_LIST: {
                auto data = std::dynamic_pointer_cast<FileListData>(request.request);

                return format_file_list(request.message_id, *data);
            }
            case CommandCode::PING: {
                auto data = std::dynamic_pointer_cast<PingData>(request.request);

                return format_ping(request.message_id, *data);
            }
            case CommandCode::DATA_PACKET: {
                auto data = std::dynamic_pointer_cast<DataPacketData>(request.request);

                return format_data_packet(request.message_id, *data);
            }
            case CommandCode::PAIR_REQUEST:
            case CommandCode::ACCEPT_PAIR_REQUEST:
                throw std::runtime_error("TODO: NOT IMPLEMENTED");
            default:
                throw std::runtime_error("UNKNOWN_COMMAND");
        }
    }

    std::size_t ProtocolHandler::parse_request(std::string_view raw_msg, Request &out) {
        if (raw_msg.size() < base_header_size)
            return 0;
        if (raw_msg.rfind(magic_bytes, 0) != 0)
            throw std::runtime_error("Missing magic bytes");

        CommandCode command_code = (CommandCode)raw_msg[4];
        std::uint8_t message_id = raw_msg[5];
        std::size_t header_size;
        Utils::VarInt payload_size;

        if (!payload_size.parse(raw_msg.substr(6, 8)))
            throw std::runtime_error("MESSAGE_TOO_LONG");
        header_size = base_header_size + payload_size.byte_size();
        if (raw_msg.size() < header_size + payload_size.to_number()) {
            return 0; // Payload is not complete, 0 bytes parsed
        }
        std::shared_ptr<IRequestData> request_data = get_request_data(command_code, raw_msg.substr(header_size, payload_size.to_number()));

        out.code = command_code;
        out.message_id = message_id;
        out.request = request_data;
        // TODO: check the whole payload_size has been parsed: if there is leftovers -> BAD_REQUEST
        return header_size + payload_size.to_number();
    }

    std::shared_ptr<IRequestData> ProtocolHandler::get_request_data(CommandCode cmd, std::string_view payload) {
        switch (cmd) {
            case CommandCode::RESPONSE:
                return parse_response(payload);
            case CommandCode::SEND_FILE:
                return parse_send_file(payload);
            case CommandCode::RECEIVE_FILE:
                return parse_receive_file(payload);
            case CommandCode::LIST_FILES:
                return parse_list_files(payload);
            case CommandCode::FILE_LIST:
                return parse_file_list(payload);
            case CommandCode::PING:
                return parse_ping(payload);
            case CommandCode::DATA_PACKET:
                return parse_data_packet(payload);
            case CommandCode::PAIR_REQUEST:
            case CommandCode::ACCEPT_PAIR_REQUEST:
                throw std::runtime_error("TODO: NOT IMPLEMENTED");
            default:
                throw std::runtime_error("UNKNOWN_COMMAND");
        }
    }

    // --------------------------------------------------------------------
    // | MAGIC_BYTES | |  COMMAND_CODE  | |  MESSAGE_ID  | | PAYLOAD_SIZE |
    // |      4      | |        1       | |       1      | |    MAX(8)    |
    // |    STRING   | |      ENUM      | |       -      | |    VARINT    |
    // --------------------------------------------------------------------
    // |    STATUS   |
    // |      1      |
    // |     ENUM    |
    // ---------------
    std::string ProtocolHandler::format_response(std::uint8_t message_id, const ResponseData &data) {
        std::string result;
        Utils::VarInt payload_size = 1;

        result.reserve(4 + 1 + 1 + payload_size.byte_size() + payload_size.to_number());
        result += magic_bytes;
        result += (char)CommandCode::RESPONSE;
        result += message_id;
        result += payload_size.to_string();
        result += (char)data.status;
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_response(std::string_view payload) {
        ResponseData data;
        Utils::VarInt varint;

        if (payload.size() < 1)
            throw std::runtime_error("BAD_REQUEST");
        data.status = (StatusCode)payload[0];
        return std::make_shared<ResponseData>(data);
    }

    // --------------------------------------------------------------------
    // | MAGIC_BYTES | |  COMMAND_CODE  | |  MESSAGE_ID  | | PAYLOAD_SIZE |
    // |      4      | |        1       | |       1      | |    MAX(8)    |
    // |    STRING   | |      ENUM      | |       -      | |    VARINT    |
    // --------------------------------------------------------------------
    // |FILEPATH_SIZE| |    FILEPATH    | | HASH_TYPE | |    FILE_HASH    |
    // |      -      | | FILEPATH_SIZE  | |     1     | |  HASH_TYPE_SIZE |
    // |   VARINT    | |     STRING     | |    ENUM   | |      STRING     |
    // --------------------------------------------------------------------
    // |  UPDATED_AT | |  PACKET_SIZE   | | TOTAL_PACKETS  |
    // |      8      | |       -        | |       -        |
    // |  SIGNED INT | |     VARINT     | |     VARINT     |
    // -----------------------------------------------------
    std::string ProtocolHandler::format_send_file(std::uint8_t message_id, const SendFileData &data) {
        std::string result;
        std::int64_t updated_at = Utils::to_epoch(data.last_updated);
        Utils::VarInt filepath_size = data.filepath.size();
        Utils::VarInt packet_size = data.packet_size;
        Utils::VarInt total_packets = data.total_packets;

        if (data.filehash.size() != Utils::algo_hash_size(data.hash_algorithm))
            throw std::runtime_error("Wrong hash size");

        Utils::VarInt payload_size = filepath_size.byte_size() + filepath_size.to_number() +
            1 + data.filehash.size() + 8 + packet_size.byte_size() + total_packets.byte_size();

        result.reserve(4 + 1 + 1 + payload_size.byte_size() + payload_size.to_number());
        result += magic_bytes;
        result += (char)CommandCode::SEND_FILE;
        result += message_id;
        result += payload_size.to_string();
        result += filepath_size.to_string();
        result += data.filepath;
        result += (std::uint8_t)data.hash_algorithm;
        result += data.filehash;
        result += Utils::serialize(updated_at);
        result += packet_size.to_string();
        result += total_packets.to_string();
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_send_file(std::string_view payload) {
        SendFileData data;
        Utils::VarInt varint;
        std::size_t algo_size;
        std::uint64_t updated_at;

        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        if (payload.size() < varint.to_number())
            throw std::runtime_error("BAD_REQUEST");
        data.filepath = payload.substr(0, varint.to_number());
        payload = payload.substr(varint.to_number());
        data.hash_algorithm = (Utils::HashAlgorithm)payload[0];
        algo_size = Utils::algo_hash_size(data.hash_algorithm);
        if (payload.size() < 1 + algo_size + 8 + 1)
            throw std::runtime_error("BAD_REQUEST");
        data.filehash = payload.substr(1, algo_size);
        payload = payload.substr(1 + algo_size);
        Utils::parse(payload.substr(0, 8), updated_at);
        data.last_updated = Utils::from_epoch<std::chrono::file_clock>(updated_at);
        payload = payload.substr(8);
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.packet_size = varint.to_number();
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.total_packets = varint.to_number();
        return std::make_shared<SendFileData>(data);
    }

    // ----------------------------------------------------------------------
    // | MAGIC_BYTES | |  COMMAND_CODE  | |  MESSAGE_ID  | |  PAYLOAD_SIZE  |
    // |      4      | |        1       | |       1      | |      MAX(8)    |
    // |    STRING   | |      ENUM      | |       -      | |      VARINT    |
    // ----------------------------------------------------------------------
    // |FILEPATH_SIZE| |    FILEPATH    | |  PACKET_SIZE | |  PACKET_START  |
    // |      -      | |  FILEPATH_SIZE | |       -      | |       -        |
    // |   VARINT    | |        -       | |    VARINT    | |     VARINT     |
    // ----------------------------------------------------------------------
    std::string ProtocolHandler::format_receive_file(std::uint8_t message_id, const ReceiveFileData &data) {
        std::string result;
        Utils::VarInt filepath_size = data.filepath.size();
        Utils::VarInt v_packet_size = packet_size;
        Utils::VarInt v_packet_start = data.packet_start;

        Utils::VarInt payload_size = filepath_size.byte_size() + filepath_size.to_number() +
            v_packet_size.byte_size() + v_packet_start.byte_size();
        result.reserve(4 + 1 + 1 + payload_size.byte_size() + payload_size.to_number());
        result += magic_bytes;
        result += (char)CommandCode::RECEIVE_FILE;
        result += message_id;
        result += payload_size.to_string();
        result += filepath_size.to_string();
        result += data.filepath;
        result += v_packet_size.to_string();
        result += v_packet_start.to_string();
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_receive_file(std::string_view payload) {
        ReceiveFileData data;
        Utils::VarInt varint;

        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        if (payload.size() < varint.to_number())
            throw std::runtime_error("BAD_REQUEST");
        data.filepath = payload.substr(0, varint.to_number());
        payload = payload.substr(varint.to_number());
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.packet_size = varint.to_number();
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.packet_start = varint.to_number();
        return std::make_shared<ReceiveFileData>(data);
    }

    // -------------------------------------------------------------------------
    // |  MAGIC_BYTES  | |   COMMAND_CODE  | |  MESSAGE_ID  | |  PAYLOAD_SIZE  |
    // |       4       | |        1        | |       1      | |     MAX(8)     |
    // |    STRING     | |       ENUM      | |       -      | |     VARINT     |
    // -------------------------------------------------------------------------
    // |FOLDERPATH_SIZE| |   FOLDERPATH    |
    // |       -       | | FOLDERPATH_SIZE |
    // |    VARINT     | |     STRING      |
    // -------------------------------------
    std::string ProtocolHandler::format_list_files(std::uint8_t message_id, const ListFilesData &data) {
        std::string result;
        Utils::VarInt folderpath_size = data.folderpath.size();

        Utils::VarInt payload_size = folderpath_size.byte_size() + folderpath_size.to_number();
        result.reserve(4 + 1 + 1 + payload_size.byte_size() + payload_size.to_number());
        result += magic_bytes;
        result += (char)CommandCode::LIST_FILES;
        result += message_id;
        result += payload_size.to_string();
        result += folderpath_size.to_string();
        result += data.folderpath;
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_list_files(std::string_view payload) {
        ListFilesData data;
        Utils::VarInt varint;

        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        if (payload.size() < varint.to_number())
            throw std::runtime_error("BAD_REQUEST");
        data.folderpath = payload.substr(0, varint.to_number());
        return std::make_shared<ListFilesData>(data);
    }

    // ------------------------------------------------------------------
    // |MAGIC_BYTES| |  COMMAND_CODE  | |  MESSAGE_ID  | | PAYLOAD_SIZE |
    // |     4     | |       1        | |      1       | |    MAX(8)    |
    // |   STRING  | |      ENUM      | |      -       | |    VARINT    |
    // ------------------------------------------------------------------
    // | REQUEST_ID| |   PACKET_ID    | |  ITEM_COUNT  |
    // |     1     | |       -        | |      -       |
    // |     -     | |     VARINT     | |    VARINT    |
    // ---------------------------------------------------------
    // |    ARRAY    [FILEPATH_SIZE,   FILEPATH   , FILE_TYPE] |
    // |  ITEM_COUNT [      -      ,    STRING    ,     1    ] |
    // |      -      [    VARINT   , FILEPATH_SIZE,    ENUM  ] |
    // ---------------------------------------------------------
    std::string ProtocolHandler::format_file_list(std::uint8_t message_id, const FileListData &data) {
        std::string result;
        std::size_t array_total_size = 0;
        Utils::VarInt item_count = data.files.size();
        Utils::VarInt v_packet_id = data.packet_id;
        Utils::VarInt payload_size = 0;

        for (const auto &file : data.files) {
            Utils::VarInt v = file.path.size();

            array_total_size += v.byte_size() + v.to_number() + 1;
        }
        payload_size = 1 + v_packet_id.byte_size() + item_count.byte_size() + array_total_size;

        result.reserve(4 + 1 + 1 + payload_size.byte_size() + payload_size.to_number());
        result += magic_bytes;
        result += (char)CommandCode::FILE_LIST;
        result += message_id;
        result += payload_size.to_string();
        result += data.request_id;
        result += v_packet_id.to_string();
        result += item_count.to_string();

        for (const auto &file : data.files) {
            Utils::VarInt v = file.path.size();

            result += v.to_string();
            result += file.path;
            result += (std::uint8_t)file.file_type;
        }
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_file_list(std::string_view payload) {
        FileListData data;
        Utils::VarInt varint;
        std::size_t nb_items;

        data.request_id = payload[0];
        payload = payload.substr(1);
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.packet_id = varint.to_number();
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        nb_items = varint.to_number();
        data.files.reserve(nb_items);
        for (std::size_t i = 0; i < nb_items; i++) {
            FileInfo file_info;

            if (!varint.parse(payload, payload))
                throw std::runtime_error("BAD_REQUEST");
            if (payload.size() < varint.to_number() + 1)
                throw std::runtime_error("BAD_REQUEST");
            file_info.path = payload.substr(0, varint.to_number());
            payload = payload.substr(varint.to_number());
            file_info.file_type = (FileType)payload[0];
            data.files.emplace_back(file_info);
            payload = payload.substr(1);
        }
        return std::make_shared<FileListData>(data);
    }

    // ---------------------------------------------------------------------
    // |  MAGIC_BYTES  | | COMMAND_CODE | |  MESSAGE_ID  | | PAYLOAD_SIZE  |
    // |       4       | |      1       | |       1      | |    MAX(8)     |
    // |    STRING     | |     ENUM     | |       -      | |    VARINT     |
    // ---------------------------------------------------------------------
    // |   REQUEST_ID  | |   PACKET_ID  | | PACKET_SIZE  | |  PACKET_DATA  |
    // |       1       | |       -      | |      -       | |  PACKET_SIZE  |
    // |       -       | |     VARINT   | |    VARINT    | |     STRING    |
    // ---------------------------------------------------------------------
    std::string ProtocolHandler::format_data_packet(std::uint8_t message_id, const DataPacketData &data) {
        std::string result;
        Utils::VarInt packet_id = data.packet_id;
        Utils::VarInt packet_size = data.data.size();

        Utils::VarInt payload_size = 1 + packet_id.byte_size() + packet_size.byte_size() +
            packet_size.to_number();
        result.reserve(4 + 1 + 1 + payload_size.byte_size());
        result += magic_bytes;
        result += (char)CommandCode::DATA_PACKET;
        result += message_id;
        result += payload_size.to_string();
        result += data.request_id;
        result += packet_id.to_string();
        result += packet_size.to_string();
        result += data.data;
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_data_packet(std::string_view payload) {
        DataPacketData data;
        Utils::VarInt varint;
        std::size_t nb_items;

        data.request_id = payload[0];
        payload = payload.substr(1);
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.packet_id = varint.to_number();
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        if (payload.size() < varint.to_number())
            throw std::runtime_error("BAD_REQUEST");
        data.data = payload.substr(0, varint.to_number());
        return std::make_shared<DataPacketData>(data);
    }

    // ------------------------------------------------------------------
    // |MAGIC_BYTES| |  COMMAND_CODE  | |  MESSAGE_ID  | | PAYLOAD_SIZE |
    // |     4     | |        1       | |       1      | |    MAX(8)    |
    // |   STRING  | |      ENUM      | |       -      | |    VARINT    |
    // ------------------------------------------------------------------
    std::string ProtocolHandler::format_ping(std::uint8_t message_id, const PingData &data) {
        std::string result;
        static const Utils::VarInt payload_size = 0;

        result.reserve(4 + 1 + 1 + payload_size.byte_size());
        result += magic_bytes;
        result += (char)CommandCode::PING;
        result += message_id;
        result += payload_size.to_string();
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_ping(std::string_view payload) {
        return std::make_shared<PingData>();
    }
}
