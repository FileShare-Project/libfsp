/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:23:57 2022 Francois Michaut
** Last update Fri Aug 22 18:59:52 2025 Francois Michaut
**
** Config.hpp : Configuration of the file sharing
*/

#pragma once

#include "FileShare/Config/FileMapping.hpp"

#include <filesystem>

namespace FileShare {
    class Config {
        public:
            enum TransportMode : std::uint8_t {
                UDP,
                TCP,
                AUTOMATIC            // AUTOMATIC switches between TCP/UDP based
                                     // on current operation and errors/latency
            };

            Config();

            // paths starting with '~/' will have this part replaced by the current user's home directory
            static auto load(std::filesystem::path config_file = "") -> Config;
            void save(std::filesystem::path config_file = "") const;

            [[nodiscard]] auto get_downloads_folder() const -> const std::filesystem::path & { return m_downloads_folder; }
            auto set_downloads_folder(const std::filesystem::path &path) -> Config &;

            auto set_file_mapping(FileMapping mapping) -> Config & { m_filemap = std::move(mapping); return *this; }
            auto get_file_mapping() const -> const FileMapping & { return m_filemap; }
            auto get_file_mapping() -> FileMapping & { return m_filemap; }

            [[nodiscard]] auto get_transport_mode() const -> TransportMode { return m_transport_mode; }
            auto set_transport_mode(TransportMode mode) -> Config & { m_transport_mode = mode; return *this; }

        private:
            template <class Archive>
            friend void serialize(Archive &archive, Config &config, std::uint32_t version);

            Config(bool);

            std::filesystem::path m_filepath;

            // Nickname for this given Peer. Allows users to override the display_name advertised by
            // Peers with a custom one. Invalid on the default Config.
            std::string m_nickname;

            // The protocol works on both TCP and UDP. You can force one or the
            // other using this option, however we recomand to keep the default.
            TransportMode m_transport_mode = AUTOMATIC;

            // List of mapped directories/files that will be available to the
            // remote clients for listing/download if visibility is PUBLIC.
            // It does not restrict what files you can send.
            //
            // However, firectories/files in the forbidden_paths will NEVER be sent
            // to other clients. Sensitive directories like ~/.ssh should be listed.
            // It will override any path set in in m_root_nodes and will also
            // apply to files you send.
            // Use this to allow certain directory but exclude sub-directories,
            // and to prevent against sentitives files/folders mistakely
            // added to m_root_nodes or manually sent.
            FileMapping m_filemap;
            // Default location for downloads. Default to a 'FileShare/' folder
            // in the local Downloads folder if empty string.
            std::filesystem::path m_downloads_folder = "";
    };
}
