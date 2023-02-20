/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:23:57 2022 Francois Michaut
** Last update Sun Feb 19 10:56:44 2023 Francois Michaut
**
** Config.hpp : Configuration of the file sharing
*/

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace FileShareProtocol {
    class Config {
        public:
            enum TransportMode {
                UDP,
                TCP,
                AUTOMATIC            // AUTOMATIC switches between TCP/UDP based
                                     // on current operation and errors/latency
            };
            static const std::filesystem::perms secure_file_perms;
            static const std::filesystem::perms secure_folder_perms;

            // paths starting with '~/' will have this part replaced by the current user's home directory
            Config(
                const std::filesystem::path &downloads_folder = "",
                std::string root_name = "//fsp",
                const std::vector<std::filesystem::path> &public_paths = {},
                const std::vector<std::filesystem::path> &private_paths = {"~/.ssh", "~/.fsp"},
                const std::filesystem::path &private_keys_dir = "~/.fsp/private",
                std::string private_key_name = "file_share",
                TransportMode transport_mode = AUTOMATIC, bool disable_server = false
            );
            ~Config() = default;

            Config(const Config &other) = default;
            Config(Config &&other) noexcept  = default;
            Config &operator=(const Config &other)  = default;
            Config &operator=(Config &&other) noexcept  = default;

            static Config from_file(std::filesystem::path config_file);
            void to_file(std::filesystem::path config_file);

            // Theses methods silently ignore missing/duplicates
            void add_public_path(std::filesystem::path path);
            void add_public_paths(std::vector<std::filesystem::path> paths);
            void remove_public_path(std::filesystem::path path);
            void remove_public_paths(std::vector<std::filesystem::path> paths);

            void add_private_path(std::filesystem::path path);
            void add_private_paths(std::vector<std::filesystem::path> paths);
            void remove_private_path(std::filesystem::path path);
            void remove_private_paths(std::vector<std::filesystem::path> paths);

            [[nodiscard]]
            const std::vector<std::filesystem::path> &get_public_paths() const;
            [[nodiscard]]
            const std::vector<std::filesystem::path> &get_private_paths() const;
            [[nodiscard]]
            const std::filesystem::path &get_downloads_folder() const;
            void set_public_paths(std::vector<std::filesystem::path> paths);
            void set_private_paths(std::vector<std::filesystem::path> paths);
            void set_downloads_folder(const std::filesystem::path path);

            [[nodiscard]]
            const std::filesystem::path &get_private_keys_dir() const;
            [[nodiscard]]
            const std::string &get_private_key_name() const;
            void set_private_keys_dir(std::filesystem::path path);
            void set_private_key_name(std::string name);

            [[nodiscard]]
            const std::string &get_root_name() const;
            [[nodiscard]]
            TransportMode get_transport_mode() const;
            void set_root_name(std::string root_name);
            void set_transport_mode(TransportMode mode);

            [[nodiscard]]
            bool is_server_disabled() const;
            void set_server_disabled(bool disabled);
        private:
            // Paths displayed to other clients are anonymised to protect user
            // privacy. root_name will be used as the virtual root prefix
            std::string m_root_name;
            // The protocol works on both TCP and UDP. You can force one or the
            // other using this option, however we recomand to keep the default.
            TransportMode m_transport_mode;

            // List of directories/files that will be available to the remote
            // client for listing/download
            // It does not restrict what files you can send.
            std::vector<std::filesystem::path> m_public_paths;
            // List of directory/files that should NEVER be sent to other
            // clients. Sensitive directories like ~/.ssh should be listed.
            // It restrict upload to protect against mistakes.
            std::vector<std::filesystem::path> m_private_paths;
            // Default location for downloads. Default to local Downloads folder
            // if empty string.
            std::filesystem::path m_downloads_folder;

            // Folder where the private key/certificate will be stored/created.
            // An error will be raised if it exists with unsecure permissions.
            std::filesystem::path m_private_keys_dir;
            // Name of the key/certificate file to use
            std::string m_private_key_name;

            // If set to true, the server will not open a socket nor listen to
            // incomming connections. You will still be able to connect to other
            // clients but they will not be able to initiate the connection.
            // Set this to true if you want an extra layer of security.
            bool m_disable_server;
    };
}