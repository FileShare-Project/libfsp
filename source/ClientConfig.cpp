/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:29:35 2022 Francois Michaut
** Last update Mon Oct 24 21:19:14 2022 Francois Michaut
**
** ClientConfig.cpp : ClientConfig implementation
*/

#include "FileShareProtocol/ClientConfig.hpp"
#include "Utils/Path.hpp"

#include <iostream>

namespace FileShareProtocol {
    const std::filesystem::perms ClientConfig::secure_file_perms = std::filesystem::perms::owner_read | std::filesystem::perms::owner_write;
    const std::filesystem::perms ClientConfig::secure_folder_perms = std::filesystem::perms::owner_all;

    ClientConfig::ClientConfig(
        const std::filesystem::path &downloads_folder, std::string root_name,
        const std::vector<std::filesystem::path> &public_paths,
        const std::vector<std::filesystem::path> &private_paths,
        const std::filesystem::path &private_keys_dir, std::string pkey_name,
        TransportProtocol transport_protocol
    ) :
        root_name(std::move(root_name)),
        public_paths(Utils::resolve_home_components(public_paths)),
        private_paths(Utils::resolve_home_components(private_paths)),
        transport_protocol(transport_protocol),
        private_keys_dir(Utils::resolve_home_component(private_keys_dir)),
        private_key_name(std::move(pkey_name)),
        downloads_folder(Utils::resolve_home_component(downloads_folder))
    {
        std::filesystem::directory_entry pkey_dir{this->private_keys_dir};

        if (pkey_dir.exists()) {
            if (!pkey_dir.is_directory())
                throw std::runtime_error("The private key/certificate path is not a directory");
#ifdef _WIN32
        // TODO: figure out security recomandations for windows
#else
            if (pkey_dir.status().permissions() != secure_folder_perms)
                throw std::runtime_error("The private key/certificate path has insecure permissions");
#endif
        }
        if (downloads_folder.empty()) {
            this->downloads_folder = Utils::resolve_home_component("~/Downloads"); // TODO: replace by cross-plateform way of getting the dowloads folder
        }
    }

    const std::filesystem::path &ClientConfig::get_private_keys_dir() const {
        return private_keys_dir;
    }

    const std::string &ClientConfig::get_private_key_name() const {
        return private_key_name;
    }

    const std::filesystem::path &ClientConfig::get_downloads_folder() const {
        return downloads_folder;
    }

}
