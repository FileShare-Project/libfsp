/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 10:37:57 2023 Francois Michaut
** Last update Sun Aug 24 11:21:02 2025 Francois Michaut
**
** ProtocolVersion.cpp : A class to represent a Protocol Version
*/

#include "FileShare/Protocol/Version.hpp"

#include <CppSockets/OSDetection.hpp>

#include <format>
#include <stdexcept>

namespace FileShare::Protocol {
    Version::Version(VersionEnum version) :
        m_version(version), m_versions({
          static_cast<std::uint8_t>((version & 0xFF0000) >> 16),
          static_cast<std::uint8_t>((version & 0x00FF00) >> 8),
          static_cast<std::uint8_t>(version & 0x0000FF)
        })
    {}

    auto Version::operator==(const Version &other) const -> bool {
        return m_version == other.m_version;
    }

    auto Version::operator<=>(const Version &other) const -> std::strong_ordering {
#ifdef OS_APPLE
        if (std::lexicographical_compare(m_versions.begin(), m_versions.end(), other.m_versions.begin(), other.m_versions.end()))
            return std::strong_ordering::less;
        else if (std::lexicographical_compare(other.m_versions.begin(), other.m_versions.end(), m_versions.begin(), m_versions.end()))
            return std::strong_ordering::greater;
        return std::strong_ordering::equal;
#else
        return m_versions <=> other.m_versions;
#endif
    }

    auto Version::to_string() const -> std::string_view {
        const auto *iter = Version::NAMES.find(this->m_version);

        if (iter == Version::NAMES.end()) {
            throw std::runtime_error(std::format("Unkown Version: {:d}", static_cast<int>(this->m_version)));
        }
        return iter->second;
    }
}
