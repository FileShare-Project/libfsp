/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Nov 19 11:23:07 2023 Francois Michaut
** Last update Wed Nov 22 22:10:24 2023 Francois Michaut
**
** FileMapping.hpp : Class to hold information about which files are available for listing/download
*/

#pragma once

#include "FileShare/Utils/StringHash.hpp"

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

namespace FileShare {
    // TODO: not protected against cyclic graph
    class PathNode {
        public:
            enum Type { HOST_FILE, HOST_FOLDER, VIRTUAL };
            enum Visibility { VISIBLE, HIDDEN };
            using NodeMap = std::unordered_map<std::string, PathNode, FileShare::Utils::string_hash, std::equal_to<>>;

            static PathNode make_virtual_node(std::string name, Visibility visibility = HIDDEN);
            static PathNode make_virtual_node(std::string name, Visibility visibility, NodeMap child_nodes);
            static PathNode make_virtual_node(std::string name, Visibility visibility, std::vector<PathNode> child_nodes);
            static PathNode make_virtual_node(std::string name, NodeMap child_nodes, Visibility visibility = HIDDEN);
            static PathNode make_virtual_node(std::string name, std::vector<PathNode> child_nodes, Visibility visibility = HIDDEN);
            static PathNode make_host_node(std::string name, Type type, std::filesystem::path host_path, Visibility visibility = HIDDEN);

            bool operator==(const PathNode &other) const noexcept;

            PathNode &add_child_node(PathNode node);
            PathNode &remove_child_node(std::string_view name);
            PathNode &clear_child_nodes();

            [[nodiscard]] std::string_view get_name() const;
            PathNode &set_name(std::string name);

            [[nodiscard]] Type get_type() const;
            PathNode &set_type(Type type);

            [[nodiscard]] Visibility get_visibility() const;
            PathNode &set_visibility(Visibility visibility);

            [[nodiscard]] NodeMap &get_child_nodes();
            [[nodiscard]] const NodeMap &get_child_nodes() const;
            PathNode &set_child_nodes(NodeMap child_nodes);
            PathNode &set_child_nodes(std::vector<PathNode> child_nodes);

            [[nodiscard]] const std::filesystem::path &get_host_path() const;
            PathNode &set_host_path(std::filesystem::path host_path);

        protected:
            PathNode(std::string name, Visibility visibility, NodeMap child_nodes);
            PathNode(std::string name, Type type, std::filesystem::path host_path, Visibility visibility = HIDDEN);

        private:
            static void assert_virtual_type(Type type);
            static void assert_not_virtual_type(Type type);
            static void assert_valid_name(std::string_view name);

        private:
            std::string m_name; // no `/` allowed in the stem
            Type m_type;
            Visibility m_visibility;
            NodeMap m_child_nodes; // Only if type == VIRTUAL
            std::filesystem::path m_host_path; // Only if type != VIRTUAL
    };

    class FileMapping {
        public:
            FileMapping(PathNode root_node);
            FileMapping(std::string root_name = default_root_name);
            FileMapping(std::string root_name, PathNode::NodeMap root_nodes);
            FileMapping(std::string root_name, std::vector<PathNode> root_nodes);
            FileMapping(PathNode::NodeMap root_nodes);
            FileMapping(std::vector<PathNode> root_nodes);

            [[nodiscard]] std::string_view get_root_name() const;
            void set_root_name(std::string root_name);

            // TODO: make sure we can't change root node visibility or type
            [[nodiscard]] const PathNode &get_root_node() const;
            [[nodiscard]] PathNode &get_root_node();
            void set_root_node(PathNode root_node);

            [[nodiscard]] const PathNode::NodeMap &get_root_nodes() const;
            [[nodiscard]] PathNode::NodeMap &get_root_nodes();
            void set_root_nodes(PathNode::NodeMap root_nodes);
            void set_root_nodes(std::vector<PathNode> root_nodes);

            std::filesystem::path host_to_virtual(const std::filesystem::path &path) const;
            std::optional<PathNode> find_virtual_node(std::filesystem::path virtual_path, bool only_visible = true) const;

            static constexpr const char *default_root_name = "//fsp";
        private:
            PathNode m_root_node;
    };
}
