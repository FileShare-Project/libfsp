/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Tue Sep 13 11:29:35 2022 Francois Michaut
** Last update Mon Aug 10 20:37:11 2026 Francois Michaut
**
** FileShareConfig.cpp : FileShareConfig implementation
*/

#include "FileShare/Config/Config.hpp"
#include "FileShare/Config/Serialization.hpp" // NOLINT(misc-include-cleaner,unused-includes)
#include "FileShare/Errors/ConfigErrors.hpp"
#include "FileShare/Utils/Path.hpp"

#include <cereal/archives/binary.hpp>

#include <filesystem>
#include <fstream>

const char * const DEFAULT_PATH = "~/.fsp/default_config";

namespace FileShare {
    Config::Config() :
        m_filepath(FileShare::Utils::resolve_home_component(DEFAULT_PATH)),
        // TODO: replace by cross-plateform way of getting the dowloads folder
        m_downloads_folder(FileShare::Utils::resolve_home_component("~/Downloads/FileShare"))
    {}

    Config::Config(bool /*_*/) {}

    auto Config::set_downloads_folder(const std::filesystem::path &path) -> Config & {
        m_downloads_folder = FileShare::Utils::resolve_home_component(path);
        return *this;
    }

    auto Config::load(std::filesystem::path config_file) -> Config {
        if (config_file.empty()) {
            config_file = DEFAULT_PATH;
        }

        Config config(false);
        config.m_filepath = FileShare::Utils::resolve_home_component(config_file);

        std::ifstream file(config.m_filepath,
                           std::ios_base::binary | std::ios_base::in);

        cereal::BinaryInputArchive archive(file);

        if (!file.is_open()) {
            throw FileShare::Errors::Config::NotFoundError(config_file.string());
        }

        archive(config);
        // TODO: Need to validate config - if someone messes up the file, we could have problems
        return config;
    }

    void Config::save(std::filesystem::path config_file) const {
        if (config_file.empty()) {
            config_file = m_filepath;
        } else {
            config_file = FileShare::Utils::resolve_home_component(config_file);
        }

        std::filesystem::create_directories(config_file.parent_path());

        std::filesystem::path tmp_file = config_file.string() + ".tmp";
        std::ofstream file(tmp_file, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
        cereal::BinaryOutputArchive archive(file);

        archive(*this);
        file.close();

        std::filesystem::rename(tmp_file.c_str(), config_file.c_str());
    }
}
