/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Fri May  5 21:35:06 2023 Francois Michaut
** Last update Tue Jul 18 09:08:14 2023 Francois Michaut
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

namespace FileShare::Protocol::Handler::v0_0_0 {
    std::size_t ProtocolHandler::parse_request(std::string_view raw_msg, Request &out) {
        if (raw_msg.rfind(magic_bytes, 0) != 0)
            throw std::runtime_error("Missing magic bytes");

        CommandCode command_code = (CommandCode)raw_msg[4];
        std::uint8_t message_id = raw_msg[5];
        Utils::VarInt payload_size;

        if (!payload_size.parse(raw_msg.substr(6, 8)))
            throw std::runtime_error("MESSAGE_TOO_LONG");
        if (raw_msg.size() < header_size + payload_size.to_number())
            return 0; // Payload is not complete, 0 bytes parsed

        std::shared_ptr<IRequestData> request_data = get_request_data(command_code, message_id, raw_msg.substr(header_size, payload_size.to_number()));

        out.code = command_code;
        out.request = request_data;
        // TODO: check the whole payload_size has been parsed: if there is leftovers -> BAD_REQUEST
        return header_size + payload_size.to_number();
    }

    std::shared_ptr<IRequestData> ProtocolHandler::get_request_data(CommandCode cmd, std::uint8_t message_id, std::string_view payload) {
        switch (cmd) {
            case CommandCode::SEND_FILE:
                return parse_send_file(message_id, payload);
            case CommandCode::RECIVE_FILE:
                return parse_receive_file(message_id, payload);
            case CommandCode::LIST_FILES:
                return parse_list_files(message_id, payload);
            case CommandCode::FILE_LIST:
                return parse_file_list(message_id, payload);
            case CommandCode::PING:
                return parse_ping(message_id, payload);
            case CommandCode::DATA_PACKET:
                return parse_data_packet(message_id, payload);
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
    // |FILEPATH_SIZE| |    FILEPATH    | | HASH_TYPE | |    FILE_HASH    |
    // |      -      | | FILEPATH_SIZE  | |     1     | |  HASH_TYPE_SIZE |
    // |   VARINT    | |     STRING     | |    ENUM   | |      STRING     |
    // --------------------------------------------------------------------
    // |  UPDATED_AT | | TOTAL_PACKETS  |
    // |      8      | |       -        |
    // |  SIGNED INT | |     VARINT     |
    // ----------------------------------
    std::string ProtocolHandler::format_send_file(std::uint8_t message_id, std::string filepath, Utils::HashAlgorithm algo) {
        std::string result;
        Utils::VarInt filepath_size = filepath.size();
        std::string file_hash = Utils::file_hash(algo, filepath);
        std::filesystem::file_time_type file_updated_at = std::filesystem::last_write_time(filepath);
        std::int64_t updated_at = Utils::to_epoch(file_updated_at);
        std::size_t file_size = std::filesystem::file_size(filepath);
        Utils::VarInt total_packets = file_size / packet_size + (file_size % packet_size == 0 ? 0 : 1);

        if (file_hash.size() != Utils::algo_hash_size(algo))
            throw std::runtime_error("Wrong hash size");

        Utils::VarInt payload_size = filepath_size.byte_size() + filepath_size.to_number() +
            1 + file_hash.size() + 8 + total_packets.byte_size();
        result.reserve(4 + 1 + 1 + payload_size.byte_size() + payload_size.to_number());
        result += magic_bytes;
        result += (char)CommandCode::SEND_FILE;
        result += message_id;
        result += payload_size.to_string();
        result += filepath_size.to_string();
        result += filepath;
        result += (std::uint8_t)algo;
        result += file_hash;
        result += Utils::serialize(updated_at);
        result += total_packets.to_string();
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_send_file(std::uint8_t message_id, std::string_view payload) {
        SendFileData data(message_id);
        Utils::VarInt varint;
        std::size_t algo_size;
        std::uint64_t updated_at;

        if (!varint.parse(payload, payload))
            throw std::runtime_error("MESSAGE_TOO_LONG");
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
    std::string ProtocolHandler::format_receive_file(std::uint8_t message_id, std::string filepath, std::size_t packet_size, std::size_t packet_start) {
        std::string result;
        Utils::VarInt filepath_size = filepath.size();
        Utils::VarInt v_packet_size = packet_size;
        Utils::VarInt v_packet_start = packet_start;

        Utils::VarInt payload_size = filepath_size.byte_size() + filepath_size.to_number() +
            v_packet_size.byte_size() + v_packet_start.byte_size();
        result.reserve(4 + 1 + 1 + payload_size.byte_size() + payload_size.to_number());
        result += magic_bytes;
        result += (char)CommandCode::RECIVE_FILE;
        result += message_id;
        result += payload_size.to_string();
        result += filepath_size.to_string();
        result += filepath;
        result += v_packet_size.to_string();
        result += v_packet_start.to_string();
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_receive_file(std::uint8_t message_id, std::string_view payload) {
        ReceiveFileData data(message_id);
        Utils::VarInt varint;

        if (!varint.parse(payload, payload))
            throw std::runtime_error("MESSAGE_TOO_LONG");
        if (payload.size() < varint.to_number())
            throw std::runtime_error("BAD_REQUEST");
        data.filepath = payload.substr(0, varint.to_number());
        payload = payload.substr(varint.to_number());
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.packed_size = varint.to_number();
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
    // |FOLDERPATH_SIZE| |   FOLDERPATH    | |    PAGE_NB   | |    PAGE_SIZE   |
    // |       -       | | FOLDERPATH_SIZE | |       -      | |        -       |
    // |    VARINT     | |     STRING      | |     VARINT   | |      VARINT    |
    // -------------------------------------------------------------------------
    std::string ProtocolHandler::format_list_files(std::uint8_t message_id, std::string folderpath, std::size_t page_idx, std::size_t page_size) {
        std::string result;
        Utils::VarInt folderpath_size = folderpath.size();
        Utils::VarInt v_page_idx = page_idx;
        Utils::VarInt v_page_size = page_size;

        Utils::VarInt payload_size = folderpath_size.byte_size() + folderpath_size.to_number() +
            v_page_idx.byte_size() + v_page_size.byte_size();
        result.reserve(4 + 1 + 1 + payload_size.byte_size() + payload_size.to_number());
        result += magic_bytes;
        result += (char)CommandCode::LIST_FILES;
        result += message_id;
        result += payload_size.to_string();
        result += folderpath_size.to_string();
        result += folderpath;
        result += v_page_idx.to_string();
        result += v_page_size.to_string();
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_list_files(std::uint8_t message_id, std::string_view payload) {
        ListFilesData data(message_id);
        Utils::VarInt varint;

        if (!varint.parse(payload, payload))
            throw std::runtime_error("MESSAGE_TOO_LONG");
        if (payload.size() < varint.to_number())
            throw std::runtime_error("BAD_REQUEST");
        data.folderpath = payload.substr(0, varint.to_number());
        payload = payload.substr(varint.to_number());
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.page_nb = varint.to_number();
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.page_size = varint.to_number();
        return std::make_shared<ListFilesData>(data);
    }

    // -------------------------------------------------------------------
    // |MAGIC_BYTES| |   COMMAND_CODE  | |  MESSAGE_ID  | | PAYLOAD_SIZE |
    // |     4     | |        1        | |       1      | |    MAX(8)    |
    // |   STRING  | |       ENUM      | |      -       | |    VARINT    |
    // -------------------------------------------------------------------
    // |TOTAL_PAGES| |   PAGE_INDEX    | |  ITEM_COUNT  |
    // |     -     | |        -        | |       -      |
    // |  VARINT   | |      VARINT     | |    VARINT    |
    // ---------------------------------------------------------
    // |    ARRAY    [FILEPATH_SIZE,   FILEPATH   , FILE_TYPE] |
    // |  ITEM_COUNT [      -      ,    STRING    ,     1    ] |
    // |      -      [    VARINT   , FILEPATH_SIZE,    ENUM  ] |
    // ---------------------------------------------------------
    std::string ProtocolHandler::format_file_list(std::uint8_t message_id, std::vector<FileInfo> files, std::size_t page_idx, std::size_t total_pages) {
        std::string result;
        std::size_t array_total_size = 0;
        Utils::VarInt item_count = files.size();
        Utils::VarInt v_page_idx = page_idx;
        Utils::VarInt v_total_pages = total_pages;
        Utils::VarInt payload_size = 0;

        for (const auto &file : files) {
            Utils::VarInt v = file.path.size();

            array_total_size += v.byte_size() + v.to_number() + 1;
        }
        payload_size = v_total_pages.byte_size() + v_page_idx.byte_size() + item_count.byte_size() + array_total_size;

        result.reserve(4 + 1 + 1 + payload_size.byte_size() + payload_size.to_number());
        result += magic_bytes;
        result += (char)CommandCode::FILE_LIST;
        result += message_id;
        result += payload_size.to_string();
        result += v_total_pages.to_string();
        result += v_page_idx.to_string();
        result += item_count.to_string();

        for (const auto &file : files) {
            Utils::VarInt v = file.path.size();

            result += v.to_string();
            result += file.path;
            result += (std::uint8_t)file.file_type;
        }
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_file_list(std::uint8_t message_id, std::string_view payload) {
        FileListData data(message_id);
        Utils::VarInt varint;
        std::size_t nb_items;

        if (!varint.parse(payload, payload))
            throw std::runtime_error("MESSAGE_TOO_LONG");
        data.total_pages = varint.to_number();
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.current_page = varint.to_number();
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

    // -------------------------------------------------------------------------
    // |  MAGIC_BYTES  | |   COMMAND_CODE  | |  MESSAGE_ID  | |  PAYLOAD_SIZE  |
    // |       4       | |        1        | |       1      | |     MAX(8)     |
    // |    STRING     | |       ENUM      | |       -      | |     VARINT     |
    // -------------------------------------------------------------------------
    // | FILEPATH_SIZE | |    FILEPATH     | |   PACKET_NB  | |   PACKET_SIZE  |
    // |       -       | |  FILEPATH_SIZE  | |       -      | |        -       |
    // |    VARINT     | |     STRING      | |     VARINT   | |      VARINT    |
    // -------------------------------------------------------------------------
    // |  PACKET_DATA  |
    // |  PACKET_SIZE  |
    // |     STRING    |
    // -----------------
    std::string ProtocolHandler::format_data_packet(std::uint8_t message_id, std::string filepath, std::size_t packet_idx, std::string_view data) {
        std::string result;
        Utils::VarInt filepath_size = filepath.size();
        Utils::VarInt packet_nb = packet_idx;
        Utils::VarInt packet_size = data.size();

        Utils::VarInt payload_size = filepath_size.byte_size() + filepath_size.to_number() + packet_nb.byte_size() + packet_size.byte_size() +
            packet_size.to_number();
        result.reserve(4 + 1 + 1 + payload_size.byte_size());
        result += magic_bytes;
        result += (char)CommandCode::DATA_PACKET;
        result += message_id;
        result += payload_size.to_string();
        result += filepath_size.to_string();
        result += filepath;
        result += packet_nb.to_string();
        result += packet_size.to_string();
        result += data;
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_data_packet(std::uint8_t message_id, std::string_view payload) {
        DataPacketData data(message_id);
        Utils::VarInt varint;
        std::size_t nb_items;

        if (!varint.parse(payload, payload))
            throw std::runtime_error("MESSAGE_TOO_LONG");
        if (payload.size() < varint.to_number())
            throw std::runtime_error("BAD_REQUEST");
        data.filepath = payload.substr(0, varint.to_number());
        payload = payload.substr(varint.to_number());
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        data.packet_id = varint.to_number();
        if (!varint.parse(payload, payload))
            throw std::runtime_error("BAD_REQUEST");
        if (payload.size() < varint.to_number())
            throw std::runtime_error("BAD_REQUEST");
        data.packet_size = varint.to_number();
        data.data = payload.substr(0, varint.to_number());
        return std::make_shared<DataPacketData>(data);
    }

    // ------------------------------------------------------------------
    // |MAGIC_BYTES| |  COMMAND_CODE  | |  MESSAGE_ID  | | PAYLOAD_SIZE |
    // |     4     | |        1       | |       1      | |    MAX(8)    |
    // |   STRING  | |      ENUM      | |       -      | |    VARINT    |
    // ------------------------------------------------------------------
    std::string ProtocolHandler::format_ping(std::uint8_t message_id) {
        std::string result;
        static const Utils::VarInt payload_size = 0;

        result.reserve(4 + 1 + 1 + payload_size.byte_size());
        result += magic_bytes;
        result += (char)CommandCode::PING;
        result += message_id;
        result += payload_size.to_string();
        return result;
    }

    std::shared_ptr<IRequestData> ProtocolHandler::parse_ping(std::uint8_t message_id, std::string_view payload) {
        return std::make_shared<PingData>(message_id);
    }
}
