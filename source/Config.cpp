/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:29:35 2022 Francois Michaut
** Last update Thu Jul 20 20:38:51 2023 Francois Michaut
**
** FileShareConfig.cpp : FileShareConfig implementation
*/

#include "FileShare/Config.hpp"
#include "FileShare/Utils/Path.hpp"

#include <CppSockets/OSDetection.hpp>

namespace FileShare {
    const std::filesystem::perms Config::secure_file_perms = std::filesystem::perms::owner_read | std::filesystem::perms::owner_write;
    const std::filesystem::perms Config::secure_folder_perms = std::filesystem::perms::owner_all;

    Config::Config(
        const std::filesystem::path &downloads_folder, std::string root_name,
        const std::vector<std::filesystem::path> &public_paths,
        const std::vector<std::filesystem::path> &private_paths,
        const std::filesystem::path &private_keys_dir, std::string pkey_name,
        TransportMode transport_mode, bool disable_server
    ) :
        m_root_name(std::move(root_name)),
        m_public_paths(FileShare::Utils::resolve_home_components(public_paths)),
        m_private_paths(FileShare::Utils::resolve_home_components(private_paths)),
        m_transport_mode(transport_mode),
        m_private_keys_dir(FileShare::Utils::resolve_home_component(private_keys_dir)),
        m_private_key_name(std::move(pkey_name)),
        m_downloads_folder(FileShare::Utils::resolve_home_component(downloads_folder)),
        m_disable_server(disable_server)
    {
        std::filesystem::directory_entry pkey_dir{m_private_keys_dir};

        if (pkey_dir.exists()) {
            if (!pkey_dir.is_directory())
                throw std::runtime_error("The private key/certificate path is not a directory");
#ifdef OS_WINDOWS
        // TODO: figure out security recomandations for windows
#else
            if (pkey_dir.status().permissions() != secure_folder_perms)
                throw std::runtime_error("The private key/certificate path has insecure permissions");
#endif
        }
        if (m_downloads_folder.empty()) {
            m_downloads_folder = FileShare::Utils::resolve_home_component("~/Downloads"); // TODO: replace by cross-plateform way of getting the dowloads folder
        }
    }

    // static Config Config::from_file(std::filesystem::path config_file);
    // Config &Config::to_file(std::filesystem::path config_file);

    // Config &Config::add_public_path(std::filesystem::path path);
    // Config &Config::add_public_paths(std::vector<std::filesystem::path> paths);
    // Config &Config::remove_public_path(std::filesystem::path path);
    // Config &Config::remove_public_paths(std::vector<std::filesystem::path> paths);

    // Config &Config::add_private_path(std::filesystem::path path);
    // Config &Config::add_private_paths(std::vector<std::filesystem::path> paths);
    // Config &Config::remove_private_path(std::filesystem::path path);
    // Config &Config::remove_private_paths(std::vector<std::filesystem::path> paths);

    // const std::vector<std::filesystem::path> &Config::get_public_paths() const;
    // const std::vector<std::filesystem::path> &Config::get_private_paths() const;
    const std::filesystem::path &Config::get_downloads_folder() const { return m_downloads_folder; }
    // Config &Config::set_public_paths(std::vector<std::filesystem::path> paths);
    // Config &Config::set_private_paths(std::vector<std::filesystem::path> paths);
    // Config &Config::set_downloads_folder(const std::filesystem::path path);

    const std::filesystem::path &Config::get_private_keys_dir() const { return m_private_keys_dir; }
    const std::string &Config::get_private_key_name() const { return m_private_key_name; }
    // Config &Config::set_private_keys_dir(std::filesystem::path path);
    // Config &Config::set_private_key_name(std::string name);

    // const std::string &Config::get_root_name() const;
    // Config::TransportMode Config::get_transport_mode() const;
    // Config &Config::set_root_name(std::string root_name);
    // Config &Config::set_transport_mode(TransportMode mode);

    bool Config::is_server_disabled() const { return m_disable_server; }
    Config &Config::set_server_disabled(bool disabled) { m_disable_server = disabled; return *this; }
}
