/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 10:37:57 2023 Francois Michaut
** Last update Tue Nov 28 13:40:51 2023 Francois Michaut
**
** ProtocolVersion.cpp : A class to represent a Protocol Version
*/

#include "FileShare/Protocol/Version.hpp"

#include <CppSockets/OSDetection.hpp>

namespace FileShare::Protocol {
    Version::Version(VersionEnum version) :
        m_version(version), m_versions({
          (std::uint8_t)((version & 0xFF0000) >> 16),
          (std::uint8_t)((version & 0x00FF00) >> 8),
          (std::uint8_t)(version & 0x0000FF)
        })
    {}

    bool Version::operator==(const Version &o) const {
        return m_version == o.m_version;
    }

    std::strong_ordering Version::operator<=>(const Version &o) const {
#ifdef OS_APPLE
        if (std::lexicographical_compare(m_versions.begin(), m_versions.end(), o.m_versions.begin(), o.m_versions.end()))
            return std::strong_ordering::less;
        else if (std::lexicographical_compare(o.m_versions.begin(), o.m_versions.end(), m_versions.begin(), m_versions.end()))
            return std::strong_ordering::greater;
        return std::strong_ordering::equal;
#else
        return m_versions <=> o.m_versions;
#endif
    }
}
