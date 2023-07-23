/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Fri May  5 19:42:09 2023 Francois Michaut
** Last update Wed Jul 19 19:54:56 2023 Francois Michaut
**
** Version.hpp : A class to represent a Protocol Version
*/

#pragma once

#include <array>
#include <compare>
#include <cstdint>

namespace FileShare::Protocol {
    class Version {
        public:
            // major version is in the highest 2hex digits
            // minor version is in the middle 2hex digits
            // patch version is in the last 2hex digits
            enum VersionEnum {
                v0_0_0 = 0x000000
                // v0_0_1 = 0x000001,
                // v0_1_0 = 0x000100
            };
            Version(VersionEnum version);

            constexpr operator VersionEnum() const { return m_version; }

            explicit operator bool() const = delete;
            bool operator==(const Version &) const;
            std::strong_ordering operator<=>(const Version&) const;

            std::uint8_t major() const { return m_versions[0]; };
            std::uint8_t minor() const { return m_versions[1]; };
            std::uint8_t patch() const { return m_versions[2]; };
        private:
            VersionEnum m_version;
            std::array<std::uint8_t, 3> m_versions;
    };
}
