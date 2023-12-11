/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:23:57 2022 Francois Michaut
** Last update Sun Dec 10 17:37:30 2023 Francois Michaut
**
** Config.hpp : Configuration of the file sharing
*/

#pragma once

#include "FileShare/Config/FileMapping.hpp"

#include <filesystem>
#include <vector>

namespace FileShare {
    class Config {
        public:
            enum TransportMode {
                UDP,
                TCP,
                AUTOMATIC            // AUTOMATIC switches between TCP/UDP based
                                     // on current operation and errors/latency
            };

            Config();
            ~Config() = default;

            // paths starting with '~/' will have this part replaced by the current user's home directory
            static Config from_file(std::filesystem::path config_file = "~/.fsp/config");
            void to_file(std::filesystem::path config_file = "~/.fsp/config");

            [[nodiscard]] const std::filesystem::path &get_downloads_folder() const;
            Config &set_downloads_folder(const std::filesystem::path path);

            Config &set_file_mapping(FileMapping mapping);
            const FileMapping &get_file_mapping() const;
            FileMapping &get_file_mapping();

            [[nodiscard]] const std::filesystem::path &get_private_keys_dir() const;
            [[nodiscard]] const std::string &get_private_key_name() const;
            Config &set_private_keys_dir(std::filesystem::path path);
            Config &set_private_key_name(std::string name);

            [[nodiscard]] TransportMode get_transport_mode() const;
            Config &set_transport_mode(TransportMode mode);

            [[nodiscard]] bool is_server_disabled() const;
            Config &set_server_disabled(bool disabled);
        public:
            static const std::filesystem::perms secure_file_perms;
            static const std::filesystem::perms secure_folder_perms;

            static const std::filesystem::path &default_private_keys_dir();
        private:
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

            // Folder where the private key/certificate will be stored/created.
            // An error will be raised if it exists with unsecure permissions.
            std::filesystem::path m_private_keys_dir = Config::default_private_keys_dir();
            // Name of the key/certificate file to use
            std::string m_private_key_name = "file_share";

            // If set to true, the server will not open a socket nor listen to
            // incomming connections. You will still be able to connect to other
            // server but they will not be able to initiate the connection.
            // Set this to true if you want an extra layer of security by
            // preventing external connections.
            // Note that when you initiate a connection to another server it will
            // be able to send commands as well.
            bool m_disable_server = false;
    };
}
