/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Dec 10 10:56:44 2023 Francois Michaut
** Last update Sun Dec 10 17:27:40 2023 Francois Michaut
**
** Serialization.hpp : FileShare Config serialization functions
*/

#pragma once

#include "FileShare/Config.hpp"

#include <cereal/cereal.hpp>
#include <cereal/types/filesystem.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/unordered_set.hpp>

static constexpr std::uint32_t file_share_config_version = 0;
static constexpr std::uint32_t file_share_file_mapping_version = 0;
static constexpr std::uint32_t file_share_path_node_version = 0;

CEREAL_CLASS_VERSION(FileShare::Config, file_share_config_version);
CEREAL_CLASS_VERSION(FileShare::FileMapping, file_share_file_mapping_version);
CEREAL_CLASS_VERSION(FileShare::RootPathNode, file_share_path_node_version);
CEREAL_CLASS_VERSION(FileShare::PathNode, file_share_path_node_version);

namespace FileShare {
    template <class Archive>
    void save(Archive &ar, const FileShare::Config &config, [[maybe_unused]] const std::uint32_t version)
    {
        ar(
            config.get_transport_mode(),
            config.get_file_mapping(),
            config.get_downloads_folder(),
            config.get_private_keys_dir(),
            config.get_private_key_name(),
            config.is_server_disabled()
        );
    }

    template <class Archive>
    void load(Archive &ar, FileShare::Config &config, const std::uint32_t version)
    {
        if (version > file_share_config_version) {
            throw std::runtime_error("Config file format is more recent than what this program supports");
        }

        FileShare::Config::TransportMode mode;
        FileShare::FileMapping mapping;
        std::filesystem::path downloads_folder;
        std::filesystem::path private_keys_dir;
        std::string private_key_name;
        bool server_disabled;

        ar(mode, mapping, downloads_folder, private_keys_dir, private_key_name, server_disabled);

        config.set_transport_mode(mode)
            .set_file_mapping(std::move(mapping))
            .set_downloads_folder(std::move(downloads_folder))
            .set_private_keys_dir(std::move(private_keys_dir))
            .set_private_key_name(std::move(private_key_name))
            .set_server_disabled(server_disabled);
    }

    template <class Archive, typename Node>
    typename std::enable_if_t<std::is_base_of_v<FileShare::PathNode, Node>, void>
    save(Archive &ar, const Node &root_node, [[maybe_unused]] const std::uint32_t version)
    {
        ar(
            root_node.get_name_str(),
            root_node.get_type(),
            root_node.get_visibility(),
            root_node.get_host_path(),
            root_node.get_child_nodes()
        );
    }

    template <class Archive, typename Node>
    typename std::enable_if_t<std::is_base_of_v<FileShare::PathNode, Node>, void>
    load(Archive &ar, Node &root_node, const std::uint32_t version)
    {
        if (version > file_share_path_node_version) {
            throw std::runtime_error("PathNode file format is more recent than what this program supports");
        }

        std::string name;
        PathNode::Type type;
        PathNode::Visibility visibility;
        std::filesystem::path host_path;
        PathNode::NodeMap child_nodes;

        ar(name, type, visibility, host_path, child_nodes);

        root_node.set_name(std::move(name));
        root_node.set_type(type);
        root_node.set_visibility(visibility);
        root_node.set_host_path(std::move(host_path));
        root_node.set_child_nodes(std::move(child_nodes));
    }

    template <class Archive>
    void save(Archive &ar, const FileShare::FileMapping &file_mapping, [[maybe_unused]] const std::uint32_t version)
    {
        ar(
            file_mapping.get_root_node(),
            file_mapping.get_forbidden_paths()
        );
    }

    template <class Archive>
    void load(Archive &ar, FileShare::FileMapping &file_mapping, const std::uint32_t version)
    {
        if (version > file_share_file_mapping_version) {
            throw std::runtime_error("FileMapping file format is more recent than what this program supports");
        }

        FileShare::RootPathNode root_node;
        FileShare::FileMapping::FilepathSet forbidden_paths;

        ar(root_node, forbidden_paths);

        file_mapping.set_root_node(std::move(root_node));
        file_mapping.set_forbidden_paths(std::move(forbidden_paths));
    }
}
