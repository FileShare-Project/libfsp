/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Mon Aug 11 10:31:09 2025 Francois Michaut
** Last update Fri Aug 15 14:38:09 2025 Francois Michaut
**
** IProtocolHandler.cpp : Functions common to all protocol versions
*/

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/Protocol/Version.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

constexpr auto shift16 = 16U;
constexpr auto shift8 = 8U;

constexpr auto mask8 = 0xFFU;
constexpr auto mask16 = 0xFF00U;
constexpr auto mask32 = 0xFF0000U;

constexpr std::uint8_t version_count = FileShare::Protocol::Version::NAMES.size();
constexpr auto &magic_bytes = FileShare::Protocol::IProtocolHandler::MAGIC_BYTES;
constexpr std::uint8_t static_bytes = 6;

using ClientVersionPayoad = std::array<char, static_bytes + (version_count * 3)>;

static consteval auto make_client_version_payload() -> ClientVersionPayoad {
    const char cmd_code = static_cast<char>(FileShare::Protocol::CommandCode::SUPPORTED_VERSIONS);
    ClientVersionPayoad ret = {0, 0, 0, 0, cmd_code, version_count};
    std::uint8_t index = static_bytes;

    std::ranges::copy(std::string_view(magic_bytes), ret.begin());
    for (const auto &iter : FileShare::Protocol::Version::NAMES) {
        ret[index] = static_cast<char>((iter.first & mask32) >> shift16);
        ret[index + 1] = static_cast<char>((iter.first & mask16) >> shift8);
        ret[index + 2] = static_cast<char>(iter.first & mask8);

        index += 3;
    }

    return ret;
}

constexpr auto client_version_payload = make_client_version_payload();

namespace FileShare::Protocol {
    auto IProtocolHandler::format_client_version_list() -> std::string_view {
        return {client_version_payload.data(), client_version_payload.size()};
    }

    auto IProtocolHandler::format_server_selected_version(Version version) -> std::string {
        std::string str = MAGIC_BYTES;

        str += static_cast<char>(CommandCode::SELECTED_VERSION);
        str += static_cast<char>((version & mask32) >> shift16);
        str += static_cast<char>((version & mask16) >> shift8);
        str += static_cast<char>(version & mask8);
        return str;
    }

    auto IProtocolHandler::parse_client_version(std::string_view raw_msg, Request &out) -> std::size_t {
        std::string_view client_begining = {client_version_payload.data(), 5};
        std::vector<Version> versions;

        if (raw_msg.size() < static_bytes) {
            return 0;
        }
        if (!raw_msg.starts_with(client_begining)) {
            throw std::runtime_error("Invalid Client Version message");
        }
        std::uint8_t nb_versions = raw_msg[5];

        if (raw_msg.size() < static_bytes + (nb_versions * 3)) {
            return 0;
        }
        versions.reserve(nb_versions);

        std::string_view versions_str = raw_msg.substr(static_bytes);
        for (std::uint8_t i = 0; i < nb_versions; i++) {
            std::uint32_t version = (static_cast<std::uint8_t>(versions_str[(i * 3) + 0]) << shift16) +
                (static_cast<std::uint8_t>(versions_str[(i * 3) + 1]) << shift8) +
                (versions_str[(i * 3) + 2]);

            versions.emplace_back(static_cast<Version::VersionEnum>(version));
        }
        out.request = std::make_shared<SupportedVersionsData>(std::move(versions));
        out.message_id = 0;
        out.code = static_cast<CommandCode>(raw_msg[4]);

        return static_bytes + (nb_versions * 3);
    }

    auto IProtocolHandler::parse_server_version(std::string_view raw_msg, Request &out) -> std::size_t {
        constexpr char cmd_byte = static_cast<char>(CommandCode::SELECTED_VERSION);

        if (raw_msg.size() < 8) {
            return 0;
        }
        if (!raw_msg.starts_with(MAGIC_BYTES) || raw_msg[4] != cmd_byte) {
            throw std::runtime_error("Invalid Server Version message");
        }

        std::uint32_t version = (static_cast<std::uint8_t>(raw_msg[5]) << shift16) +
            (static_cast<std::uint8_t>(raw_msg[6]) << shift8) +
            (raw_msg[7]);

        out.request = std::make_shared<SelectedVersionData>(static_cast<Version::VersionEnum>(version));
        out.message_id = 0;
        out.code = static_cast<CommandCode>(raw_msg[4]);

        return 8;
    }
}
