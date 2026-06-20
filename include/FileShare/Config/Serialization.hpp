/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Dec 10 10:56:44 2023 Francois Michaut
** Last update Fri Jun 12 03:23:08 2026 Francois Michaut
**
** Serialization.hpp : FileShare Config serialization functions
*/

#pragma once

#include "CppSockets/Address.hpp"
#include "CppSockets/IPv4.hpp"
#include "FileShare/Config/Config.hpp"
#include "FileShare/Config/ServerConfig.hpp"

#include <cereal/access.hpp>
#include <cereal/cereal.hpp>
#include <cereal/types/filesystem.hpp>
#include <cereal/types/polymorphic.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/unordered_set.hpp>

static constexpr std::uint32_t FILE_SHARE_CONFIG_VERSION = 0;
static constexpr std::uint32_t FILE_SHARE_SERVER_CONFIG_VERSION = 1;
static constexpr std::uint32_t FILE_SHARE_FILE_MAPPING_VERSION = 0;
static constexpr std::uint32_t FILE_SHARE_PATH_NODE_VERSION = 0;
static constexpr std::uint32_t CPP_SOCKETS_ENDPOINT_VERSION = 0;

CEREAL_CLASS_VERSION(FileShare::Config, FILE_SHARE_CONFIG_VERSION); // NOLINT
CEREAL_CLASS_VERSION(FileShare::ServerConfig, FILE_SHARE_SERVER_CONFIG_VERSION); // NOLINT
CEREAL_CLASS_VERSION(FileShare::FileMapping, FILE_SHARE_FILE_MAPPING_VERSION); // NOLINT
CEREAL_CLASS_VERSION(FileShare::RootPathNode, FILE_SHARE_PATH_NODE_VERSION); // NOLINT
CEREAL_CLASS_VERSION(FileShare::PathNode, FILE_SHARE_PATH_NODE_VERSION); // NOLINT
CEREAL_CLASS_VERSION(CppSockets::EndpointV4, CPP_SOCKETS_ENDPOINT_VERSION); // NOLINT

CEREAL_REGISTER_POLYMORPHIC_RELATION(CppSockets::IEndpoint, CppSockets::EndpointV4);

// TODO: Make a custom cereal archive for .conf files : https://uscilab.github.io/cereal/serialization_archives.html#adding-more-archives
// Use .conf files for Config and ServerConfig
// Serialize UUID as a base64 encoding of the binary representation so its not easily editable

namespace FileShare {
    template <class Archive>
    void serialize(Archive &archive, FileShare::ServerConfig &config, const std::uint32_t version) {
        if (version > FILE_SHARE_SERVER_CONFIG_VERSION) {
            throw std::runtime_error("ServerConfig file format is more recent than what this program supports");
        }

        if (version == FILE_SHARE_SERVER_CONFIG_VERSION) {
            archive(
                config.m_uuid, config.m_device_name, config.m_private_keys_dir,
                config.m_private_key_name, config.m_disable_server, config.m_server_endpoint
            );
        } else if (version == 0) {
            // Defaults Server Endpoint
            archive(
                config.m_uuid, config.m_device_name, config.m_private_keys_dir,
                config.m_private_key_name, config.m_disable_server
            );
        } else {
            throw std::runtime_error("ServerConfig file format is unsupported");
        }
    }

    template <class Archive>
    void serialize(Archive &archive, FileShare::Config &config, const std::uint32_t version) {
        if (version > FILE_SHARE_CONFIG_VERSION) {
            throw std::runtime_error("Config file format is more recent than what this program supports");
        }

        if (version == FILE_SHARE_CONFIG_VERSION) {
            archive(config.m_transport_mode, config.m_filemap, config.m_downloads_folder);
        } else {
            throw std::runtime_error("Config file format is unsupported");
        }
    }

    template <class Archive, typename Node>
    void serialize(Archive &archive, Node &node, const std::uint32_t version)
    requires IsPathNode<Node> {
        if (version > FILE_SHARE_PATH_NODE_VERSION) {
            throw std::runtime_error("PathNode file format is more recent than what this program supports");
        }

        if (version == FILE_SHARE_PATH_NODE_VERSION) {
            archive(node.m_name, node.m_type, node.m_visibility, node.m_host_path, node.m_child_nodes);
        } else {
            throw std::runtime_error("PathNode file format is unsupported");
        }
    }

    template <class Archive>
    void serialize(Archive &archive, FileShare::FileMapping &file_mapping, const std::uint32_t version) {
        if (version > FILE_SHARE_FILE_MAPPING_VERSION) {
            throw std::runtime_error("FileMapping file format is more recent than what this program supports");
        }

        if (version == FILE_SHARE_FILE_MAPPING_VERSION) {
            archive(file_mapping.m_root_node, file_mapping.m_forbidden_paths);
        } else {
            throw std::runtime_error("FileMapping file format is unsupported");
        }
    }
}

namespace cereal {
    template <class Archive>
    void serialize(Archive &archive, CppSockets::EndpointV4 &endpoint, const std::uint32_t version) {
        if (version > CPP_SOCKETS_ENDPOINT_VERSION) {
            throw std::runtime_error("Endpoint file format is more recent than what this program supports");
        }

        if (version == CPP_SOCKETS_ENDPOINT_VERSION) {
            archive(endpoint.get_addr().to_string(), endpoint.get_port());
        } else {
            throw std::runtime_error("FileMapping file format is unsupported");
        }
    }

    template <> struct LoadAndConstruct<CppSockets::EndpointV4> {
        template <class Archive>
        static void load_and_construct(Archive &archive, cereal::construct<CppSockets::EndpointV4> &construct,  std::uint32_t const version) {
            if (version > CPP_SOCKETS_ENDPOINT_VERSION) {
                throw std::runtime_error("Endpoint file format is more recent than what this program supports");
            }

            if (version == CPP_SOCKETS_ENDPOINT_VERSION) {
                std::string addr;
                std::uint16_t port;

                archive(addr, port);
                CppSockets::IPv4 ipv4(addr.c_str());

                construct(ipv4, port);
            } else {
                throw std::runtime_error("FileMapping file format is unsupported");
            }

        }
    };
}
