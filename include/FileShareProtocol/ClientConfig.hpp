/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:23:57 2022 Francois Michaut
** Last update Mon Oct 24 21:18:55 2022 Francois Michaut
**
** ClientConfig.hpp : Configuration of the Client
*/

#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace FileShareProtocol {
    class ClientConfig {
        public:
            enum TransportProtocol {
                UDP,
                TCP,
                AUTOMATIC            // AUTOMATIC switches between TCP/UDP based
                                     // on current operation and errors/latency
            };
            static const std::filesystem::perms secure_file_perms;
            static const std::filesystem::perms secure_folder_perms;

            // paths starting with '~/' will have this part replaced by the current user's home directory
            ClientConfig(
                const std::filesystem::path &downloads_folder = "",
                std::string root_name = "//fsp",
                const std::vector<std::filesystem::path> &public_paths = {},
                const std::vector<std::filesystem::path> &private_paths = {"~/.ssh", "~/.fsp"},
                const std::filesystem::path &private_keys_dir = "~/.fsp/private",
                std::string private_key_name = "file_share",
                TransportProtocol transport_protocol = AUTOMATIC
            );
            ~ClientConfig() = default;

            ClientConfig(const ClientConfig &other) = default;
            ClientConfig(ClientConfig &&other) noexcept  = default;
            ClientConfig &operator=(const ClientConfig &other)  = default;
            ClientConfig &operator=(ClientConfig &&other) noexcept  = default;

            [[nodiscard]]
            const std::filesystem::path &get_downloads_folder() const;

            // Theses methods silently ignore missing/duplicates
            void add_public_path(std::filesystem::path path);
            void add_public_paths(std::vector<std::filesystem::path> paths);
            void remove_public_path(std::filesystem::path path);
            void remove_public_paths(std::vector<std::filesystem::path> paths);
            [[nodiscard]]
            std::vector<std::filesystem::path> &get_public_paths();
            [[nodiscard]]
            const std::vector<std::filesystem::path> &get_public_paths() const;
            [[nodiscard]]
            const std::filesystem::path &get_private_keys_dir() const;
            [[nodiscard]]
            const std::string &get_private_key_name() const;
        private:
            // Paths displayed to other clients are anonymised to protect user
            // privacy. root_name will be used as the virtual root prefix
            std::string root_name;
            // List of directories/files that will be available to the remote
            // client for listing/download
            // It does not restrict what files you can send.
            std::vector<std::filesystem::path> public_paths;
            // List of directory/files that should NEVER be sent to other
            // clients. Sensitive directories like ~/.ssh should be listed.
            // It restrict upload to protect against mistakes.
            std::vector<std::filesystem::path> private_paths;
            // Default location for downloads. Default to local Downloads folder
            // if empty string.
            std::filesystem::path downloads_folder;
            // Folder where the private key/certificate will be stored/created.
            // An error will be raised if it exists with unsecure permissions.
            std::filesystem::path private_keys_dir;
            // Name of the key/certificate file to use
            std::string private_key_name;
            TransportProtocol transport_protocol;
    };
}
