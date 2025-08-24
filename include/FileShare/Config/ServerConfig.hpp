/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Wed Aug  6 15:09:50 2025 Francois Michaut
** Last update Fri Aug 22 20:41:53 2025 Francois Michaut
**
** ServerConfig.hpp : Server Configuration
*/

#pragma once

#include <filesystem>

namespace FileShare {
    class ServerConfig {
        public:
            ServerConfig();

            static constexpr std::filesystem::perms SECURE_FILE_PERMS = std::filesystem::perms::owner_read | std::filesystem::perms::owner_write;
            static constexpr std::filesystem::perms SECURE_FOLDER_PERMS = std::filesystem::perms::owner_all;

            static auto default_private_keys_dir() -> const std::filesystem::path &;

            // paths starting with '~/' will have this part replaced by the current user's home directory
            static auto load(std::filesystem::path config_file = "") -> ServerConfig;
            void save(std::filesystem::path config_file = "") const;

            [[nodiscard]] auto get_uuid() const -> const std::string & { return m_uuid; }
            [[nodiscard]] auto get_device_name() const -> const std::string & { return m_device_name; }
            auto set_device_name(std::string device_name) -> ServerConfig & { m_device_name = std::move(device_name); return *this; }

            [[nodiscard]] auto get_private_keys_dir() const -> const std::filesystem::path & { return m_private_keys_dir; }
            [[nodiscard]] auto get_private_key_name() const -> const std::string & { return m_private_key_name; }
            auto set_private_keys_dir(std::string_view path) -> ServerConfig &;
            auto set_private_key_name(std::string name) -> ServerConfig &;

            [[nodiscard]] auto is_server_disabled() const -> bool { return m_disable_server; }
            auto set_server_disabled(bool disabled) -> ServerConfig & { m_disable_server = disabled; return *this; }

            void validate_config() const;
        private:
            template <class Archive>
            friend void serialize(Archive &archive, ServerConfig &config, std::uint32_t version);

            ServerConfig(std::string uuid);

            std::filesystem::path m_filepath;

            // Unique ID for this device. Needs to be globally unique. Will be generated internally.
            std::string m_uuid;
            // How this device will appear to others. Not required to be totally unique,
            // but should be unique amongst the devices you own.
            // Errors will be generated if that's not the case.
            std::string m_device_name;

            // Folder where the private key/certificate will be stored/created.
            // An error will be raised if it exists with unsecure permissions.
            std::filesystem::path m_private_keys_dir = ServerConfig::default_private_keys_dir();
            // Name of the key/certificate file to use (without extension/prefix - will be added automatically)
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
