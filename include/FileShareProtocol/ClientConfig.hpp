/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:23:57 2022 Francois Michaut
** Last update Wed Sep 14 22:25:42 2022 Francois Michaut
**
** ClientConfig.hpp : Configuration of the Client
*/

#pragma once

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

            ClientConfig(
                std::string default_destination_folder = "",
                std::string root_name = "//fsp",
                std::vector<std::string> public_paths = {},
                std::vector<std::string> private_paths = {"~/.ssh"},
                TransportProtocol transport_protocol = AUTOMATIC
            );
            ~ClientConfig() = default;

            ClientConfig(const ClientConfig &other) = default;
            ClientConfig(ClientConfig &&other) noexcept  = default;
            ClientConfig &operator=(const ClientConfig &other)  = default;
            ClientConfig &operator=(ClientConfig &&other) noexcept  = default;

            static ClientConfig &get_default();

            [[nodiscard]]
            bool is_default() const;

            // Theses methods silently ignore missing/duplicates
            void add_public_path(std::string path);
            void add_public_paths(std::vector<std::string> paths);
            void remove_public_path(std::string path);
            void remove_public_paths(std::vector<std::string> paths);
            std::vector<std::string> &get_public_paths();
            [[nodiscard]]
            const std::vector<std::string> &get_public_paths() const;
        private:
             // Paths displayed to other clients are anonymised to protect user
             // privacy. root_name will be used as the virtual root prefix
            std::string root_name;
            // List of directories/files that will be available to the remote
            // client for listing/download
            // It does not restrict what files you can send.
            std::vector<std::string> public_paths;
            // List of directory/files that should NEVER be sent to other
            // clients. Sensitive directories like ~/.ssh should be listed.
            // It does restrict upload as well to protect against mistakes.
            std::vector<std::string> private_paths;
            // Default location for downloads. Default to local Downloads folder
            // if empty string.
            std::string default_destination_folder;
            TransportProtocol transport_protocol;
            static ClientConfig default_config; //NOLINT
    };
}
