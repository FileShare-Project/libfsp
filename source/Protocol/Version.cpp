/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 10:37:57 2023 Francois Michaut
** Last update Tue May  9 08:55:51 2023 Francois Michaut
**
** ProtocolVersion.cpp : A class to represent a Protocol Version
*/

#include "FileShare/Protocol/Version.hpp"

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
        return m_versions <=> o.m_versions;
    }
}
