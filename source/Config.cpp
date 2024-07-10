/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:29:35 2022 Francois Michaut
** Last update Sun Dec 10 17:46:39 2023 Francois Michaut
**
** FileShareConfig.cpp : FileShareConfig implementation
*/

#include "FileShare/Config.hpp"
#include "FileShare/Config/Serialization.hpp"
#include "FileShare/Utils/Path.hpp"

#include <CppSockets/OSDetection.hpp>

#include <cereal/archives/binary.hpp>

#include <fstream>

namespace FileShare {
    const std::filesystem::perms Config::secure_file_perms = std::filesystem::perms::owner_read | std::filesystem::perms::owner_write;
    const std::filesystem::perms Config::secure_folder_perms = std::filesystem::perms::owner_all;

    Config::Config() {
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
            m_downloads_folder = FileShare::Utils::resolve_home_component("~/Downloads/FileShare"); // TODO: replace by cross-plateform way of getting the dowloads folder
        }
    }

    Config Config::from_file(std::filesystem::path config_file) {
        config_file = FileShare::Utils::resolve_home_component(config_file);
        Config new_config;
        std::ifstream file(config_file, std::ios_base::binary | std::ios_base::in);
        cereal::BinaryInputArchive ar(file);

        ar(new_config);
        return new_config;
    }

    void Config::to_file(std::filesystem::path config_file) {
        config_file = FileShare::Utils::resolve_home_component(config_file);
        std::ofstream file(config_file, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
        cereal::BinaryOutputArchive ar(file);

        ar(*this);
    }

    const std::filesystem::path &Config::get_downloads_folder() const { return m_downloads_folder; }
    Config &Config::set_downloads_folder(std::filesystem::path path) { m_downloads_folder = std::move(path); return *this; }

    const std::filesystem::path &Config::get_private_keys_dir() const { return m_private_keys_dir; }
    const std::string &Config::get_private_key_name() const { return m_private_key_name; }
    Config &Config::set_private_keys_dir(std::filesystem::path path) { m_private_keys_dir = std::move(path); return *this; }
    Config &Config::set_private_key_name(std::string name) { m_private_key_name = std::move(name); return *this; }

    Config &Config::set_file_mapping(FileMapping mapping) { m_filemap = mapping; return *this; }
    const FileMapping &Config::get_file_mapping() const { return m_filemap; }
    FileMapping &Config::get_file_mapping() { return m_filemap; }

    Config::TransportMode Config::get_transport_mode() const { return m_transport_mode; }
    Config &Config::set_transport_mode(TransportMode mode) { m_transport_mode = mode; return *this; }

    bool Config::is_server_disabled() const { return m_disable_server; }
    Config &Config::set_server_disabled(bool disabled) { m_disable_server = disabled; return *this; }

    const std::filesystem::path &Config::default_private_keys_dir() {
        static std::filesystem::path private_keys_dir = FileShare::Utils::resolve_home_component("~/.fsp/private").generic_string();

        return private_keys_dir;
    }
}
