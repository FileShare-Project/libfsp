/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Wed Aug  6 15:19:24 2025 Francois Michaut
** Last update Sat Jun 20 09:42:39 2026 Francois Michaut
**
** ServerConfig.cpp : Server Configuration Implementation
*/

#include "CppSockets/IPv4.hpp"
#include "FileShare/Config/Serialization.hpp" // NOLINT(misc-include-cleaner,unused-includes)
#include "FileShare/Config/ServerConfig.hpp"
#include "FileShare/Errors/ConfigErrors.hpp"
#include "FileShare/Utils/Path.hpp"

#include <CppSockets/OSDetection.hpp>

#include <cereal/archives/binary.hpp>

#include <fstream>
#include <stdexcept>

// Need to be registered after we include the archives it will be used with
CEREAL_REGISTER_TYPE(CppSockets::EndpointV4);

const char * const DEFAULT_PATH = "~/.fsp/server_config";

namespace FileShare {
    ServerConfig::ServerConfig() :
        m_filepath(FileShare::Utils::resolve_home_component(DEFAULT_PATH)),
        m_uuid("0000-0000-0000-0000"), // TODO: Generate Random UUID
        m_device_name("Unnamed Device") // TODO: Prevent this from happening. Either generate a random name or ask the user to input one on first launch
                                        // Currently only happens if default_config arg is used
    {}

    ServerConfig::ServerConfig(std::string uuid) : m_uuid(std::move(uuid)) {}

    auto ServerConfig::default_private_keys_dir() -> const std::filesystem::path & {
        static const std::filesystem::path private_keys_dir = FileShare::Utils::resolve_home_component("~/.fsp/private").generic_string();

        return private_keys_dir;
    }

    auto ServerConfig::default_endpoint() -> std::shared_ptr<CppSockets::IEndpoint> {
        // TODO: choose a better port than 12345
        return std::make_shared<CppSockets::EndpointV4>(CppSockets::IPv4("0.0.0.0"), 12345);
    }

    auto ServerConfig::set_private_keys_dir(std::string_view path) -> ServerConfig & {
        m_private_keys_dir = FileShare::Utils::resolve_home_component(path);
        return *this;
    }

    auto ServerConfig::set_private_key_name(std::string name) -> ServerConfig & {
        if (name.find('/') != std::string::npos) { // TODO: Do this properly for Windows
            throw std::runtime_error("Private Key Name cannot contain a directory separator");
        }
        m_private_key_name = std::move(name);
        return *this;
    }

    auto ServerConfig::load(std::filesystem::path config_file) -> ServerConfig {
        if (config_file.empty()) {
            config_file = DEFAULT_PATH;
        }

        // Avoid the default constructor which would wastefuly generate an UUID
        ServerConfig config("0000-0000-0000-0000");
        config.m_filepath = FileShare::Utils::resolve_home_component(config_file);
        std::ifstream file(config.m_filepath,
                           std::ios_base::binary | std::ios_base::in);
        cereal::BinaryInputArchive archive(file);

        if (!file.is_open()) {
            throw FileShare::Errors::Config::NotFoundError(config_file.string());
        }

        archive(config);
        // TODO: Need to validate config - if someone messes up the file, we could have problems
        // (Or if private_keys_dir contains ~, it needs to be expanded)
        return config;
    }

    void ServerConfig::save(std::filesystem::path config_file) const {
        if (config_file.empty()) {
            config_file = m_filepath;
        } else {
            config_file = FileShare::Utils::resolve_home_component(config_file);
        }

        std::filesystem::path tmp_file = config_file.string() + ".tmp";
        std::ofstream file(tmp_file, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
        cereal::BinaryOutputArchive archive(file);

        archive(*this);
        file.close();

        std::filesystem::rename(tmp_file.c_str(), config_file.c_str());
    }

    void ServerConfig::validate_config() const {
        const std::filesystem::directory_entry pkey_dir{m_private_keys_dir};

        if (pkey_dir.exists()) {
            if (!pkey_dir.is_directory())
                throw std::runtime_error("The private key/certificate path is not a directory");
#ifdef OS_WINDOWS
        // TODO: figure out security recomandations for windows
#else
            if (pkey_dir.status().permissions() != SECURE_FOLDER_PERMS)
                throw std::runtime_error("The private key/certificate path has insecure permissions");
#endif
        }
    }
}
