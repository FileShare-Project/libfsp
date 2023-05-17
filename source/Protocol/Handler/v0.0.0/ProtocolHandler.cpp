/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Fri May  5 21:35:06 2023 Francois Michaut
** Last update Mon May 15 11:55:42 2023 Francois Michaut
**
** ProtocolHandler.cpp : ProtocolHandler for the v0.0.0 of the protocol
*/

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Protocol/Handler/v0.0.0/ProtocolHandler.hpp"
#include "FileShare/Utils/Serialize.hpp"
#include "FileShare/Utils/VarInt.hpp"

#include <chrono>

namespace FileShare::Protocol::Handler::v0_0_0 {
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
        std::int64_t updated_at = std::chrono::duration_cast<std::chrono::seconds>(file_updated_at.time_since_epoch()).count(); // TODO: see if this is portable / reliable
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
            result += (std::uint8_t)file.tile_type;
        }
        return result;
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
}
