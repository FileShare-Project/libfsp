/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Fri May  5 19:42:09 2023 Francois Michaut
** Last update Sun Aug 24 11:20:56 2025 Francois Michaut
**
** Version.hpp : A class to represent a Protocol Version
*/

#pragma once

#include <frozen/string.h>
#include <frozen/unordered_map.h>

#include <array>
#include <compare>
#include <cstdint>

namespace FileShare::Protocol {
    class Version {
        public:
            // major version is in the highest 2hex digits
            // minor version is in the middle 2hex digits
            // patch version is in the last 2hex digits
            enum VersionEnum : std::uint32_t {
                v0_0_0 = 0x000000,
                // v0_0_1 = 0x000001,
                // v0_1_0 = 0x000100,

                MIN = v0_0_0,
                MAX = v0_0_0
            };
            Version(VersionEnum version);

            constexpr operator VersionEnum() const { return m_version; }

            explicit operator bool() const = delete;
            auto operator==(const Version &) const -> bool;
            auto operator<=>(const Version&) const -> std::strong_ordering;

            [[nodiscard]] auto major() const -> std::uint8_t { return m_versions[0]; };
            [[nodiscard]] auto minor() const -> std::uint8_t { return m_versions[1]; };
            [[nodiscard]] auto patch() const -> std::uint8_t { return m_versions[2]; };

            [[nodiscard]] auto to_string() const -> std::string_view;

            inline static constexpr auto NAMES = frozen::make_unordered_map<VersionEnum, std::string_view>({
                {v0_0_0, "v0.0.0"}
            });
        private:
            VersionEnum m_version;
            std::array<std::uint8_t, 3> m_versions;
    };
}
