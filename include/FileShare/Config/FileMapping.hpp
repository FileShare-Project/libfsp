/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Nov 19 11:23:07 2023 Francois Michaut
** Last update Sun Dec 10 17:33:23 2023 Francois Michaut
**
** FileMapping.hpp : Class to hold information about which files are available for listing/download
*/

#pragma once

#include "FileShare/Utils/StringHash.hpp"

#include <filesystem>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace FileShare {
    // TODO: not protected against cyclic graph
    class PathNode {
        public:
            enum Type { HOST_FILE, HOST_FOLDER, VIRTUAL };
            enum Visibility { VISIBLE, HIDDEN };
            using NodeMap = std::unordered_map<std::string, PathNode, FileShare::Utils::string_hash, std::equal_to<>>;

            PathNode() = default;
            virtual ~PathNode() = default;

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
            [[nodiscard]] const std::string &get_name_str() const;
            PathNode &set_name(std::string name);

            [[nodiscard]] Type get_type() const;
            PathNode &set_type(Type type);

            bool is_virtual() const;
            bool is_host() const;
            bool is_host_file() const;
            bool is_host_folder() const;

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
            virtual void assert_valid_name(std::string_view name);

        private:
            std::string m_name; // no `/` allowed in the stem
            Type m_type;
            Visibility m_visibility;
            NodeMap m_child_nodes; // Only if type == VIRTUAL
            std::filesystem::path m_host_path; // Only if type != VIRTUAL
    };

    class RootPathNode : public PathNode {
        public:
            RootPathNode(std::string root_name = default_root_name, PathNode::NodeMap root_nodes = {});
            RootPathNode(std::string root_name, std::vector<PathNode> root_nodes);
            RootPathNode(PathNode::NodeMap root_nodes);
            RootPathNode(std::vector<PathNode> root_nodes);

            static constexpr const char *default_root_name = "//fsp";
        private:
            void assert_valid_name(std::string_view name) override;
    };

    class FileMapping {
        public:
            using FilepathSet=std::unordered_set<std::filesystem::path, FileShare::Utils::string_hash, std::equal_to<>>;

            FileMapping() = default;
            FileMapping(RootPathNode root_node, FilepathSet forbidden_paths = FileMapping::default_forbidden_paths());

            [[nodiscard]] std::string_view get_root_name() const;
            void set_root_name(std::string root_name);

            // TODO: make sure we can't change root node visibility or type
            [[nodiscard]] const RootPathNode &get_root_node() const;
            [[nodiscard]] RootPathNode &get_root_node();
            void set_root_node(RootPathNode root_node);

            [[nodiscard]] const PathNode::NodeMap &get_root_nodes() const;
            [[nodiscard]] PathNode::NodeMap &get_root_nodes();
            void set_root_nodes(PathNode::NodeMap root_nodes);
            void set_root_nodes(std::vector<PathNode> root_nodes);

            [[nodiscard]] const FilepathSet &get_forbidden_paths() const;
            [[nodiscard]] FilepathSet get_forbidden_paths();
            void set_forbidden_paths(FilepathSet paths);

            std::filesystem::path host_to_virtual(const std::filesystem::path &path) const;
            std::filesystem::path virtual_to_host(const std::filesystem::path &path) const;
            std::filesystem::path virtual_to_host(const std::filesystem::path &virtual_path, const std::optional<PathNode> &node, std::filesystem::path::iterator iter) const;
            std::optional<PathNode> find_virtual_node(std::filesystem::path virtual_path, std::filesystem::path::iterator &out, bool only_visible = true) const;
            std::optional<PathNode> find_virtual_node(std::filesystem::path virtual_path, bool only_visible = true) const;

            bool is_forbidden(const std::filesystem::path &path) const;
        private:
            static const FilepathSet &default_forbidden_paths();

        private:
            RootPathNode m_root_node;
            FilepathSet m_forbidden_paths = FileMapping::default_forbidden_paths();
    };
}
