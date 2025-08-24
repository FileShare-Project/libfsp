/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Nov 19 11:23:07 2023 Francois Michaut
** Last update Sun Aug 24 11:28:31 2025 Francois Michaut
**
** FileMapping.hpp : Class to hold information about which files are available for listing/download
*/

#pragma once

#include "FileShare/Utils/Strings.hpp"

#include <cereal/types/concepts/pair_associative_container.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace FileShare {
    class FileMapping;
    class PathNode;

    template <typename Node>
    concept IsPathNode = std::is_base_of_v<FileShare::PathNode, Node>;

    class PathNode {
        public:
            enum Type : std::uint8_t { HOST_FILE, HOST_FOLDER, VIRTUAL };
            enum Visibility : std::uint8_t { VISIBLE, HIDDEN };
            using NodeMap = std::unordered_map<std::string, PathNode, FileShare::Utils::string_hash, std::equal_to<>>;

            PathNode() = default;

            PathNode(const PathNode &);
            PathNode(PathNode &&) = default;
            auto operator=(const PathNode &) -> PathNode &;
            auto operator=(PathNode &&) -> PathNode & = default;

            virtual ~PathNode() = default;

            auto operator==(const PathNode &other) const noexcept -> bool;

            static auto make_virtual_node(std::string name, Visibility visibility = HIDDEN) -> PathNode;
            static auto make_virtual_node(std::string name, Visibility visibility, NodeMap child_nodes) -> PathNode;
            static auto make_virtual_node(std::string name, Visibility visibility, std::vector<PathNode> child_nodes) -> PathNode;
            static auto make_virtual_node(std::string name, NodeMap child_nodes, Visibility visibility = HIDDEN) -> PathNode;
            static auto make_virtual_node(std::string name, std::vector<PathNode> child_nodes, Visibility visibility = HIDDEN) -> PathNode;
            static auto make_host_node(std::string name, Type type, std::filesystem::path host_path, Visibility visibility = HIDDEN) -> PathNode;

            [[nodiscard]] auto insert_child_node(PathNode node) -> PathNode &; // Return the inserted node instead of *this
            auto add_child_node(PathNode node) -> PathNode &;
            auto remove_child_node(const std::string &name) -> PathNode &;
            auto clear_child_nodes() -> PathNode &;

            [[nodiscard]] auto get_name() const -> std::string_view { return m_name; }
            virtual auto set_name(std::string name) -> PathNode &;

            [[nodiscard]] auto get_type() const -> Type { return m_type; }
            virtual auto set_type(Type type) -> PathNode &;

            auto is_virtual() const -> bool { return m_type == Type::VIRTUAL; }
            auto is_host() const -> bool { return !is_virtual(); }
            auto is_host_file() const -> bool { return m_type == Type::HOST_FILE; }
            auto is_host_folder() const -> bool { return m_type == Type::HOST_FOLDER; }

            [[nodiscard]] auto get_visibility() const -> Visibility { return m_visibility; }
            virtual auto set_visibility(Visibility visibility) -> PathNode & { m_visibility = visibility; return *this; }

            [[nodiscard]] auto get_child_nodes() const -> const NodeMap & { return m_child_nodes; }
            auto set_child_nodes(NodeMap child_nodes) -> PathNode &;
            auto set_child_nodes(std::vector<PathNode> child_nodes) -> PathNode &;

            [[nodiscard]] auto get_host_path() const -> const std::filesystem::path & { return m_host_path; }
            virtual auto set_host_path(std::filesystem::path host_path) -> PathNode &;

            auto get_ancestor_path() const -> std::filesystem::path;
        protected:
            PathNode(std::string name, PathNode *parent, Visibility visibility, NodeMap child_nodes);
            PathNode(std::string name, PathNode *parent, Type type, std::filesystem::path host_path, Visibility visibility = HIDDEN);

            template <class Archive, typename Node>
            friend void serialize(Archive &archive, Node &node, std::uint32_t version)
                requires IsPathNode<Node>;

            template <class Archive, template <typename...> class Map, typename... Args, typename mapped_type> inline
            friend void cereal::CEREAL_LOAD_FUNCTION_NAME(Archive &archive, Map<Args...> &map);

        private:
            static void assert_virtual_type(Type type);
            static void assert_not_virtual_type(Type type);
            virtual void assert_valid_name(std::string_view name);
            void assign_parent(PathNode *);
            void reassign_childs();

            std::string m_name; // no `/` allowed in the stem
            PathNode *m_parent = nullptr; // Always present, except for RootPathNode where it is always NULL
            NodeMap m_child_nodes; // Only if type == VIRTUAL
            std::filesystem::path m_host_path; // Only if type != VIRTUAL

            // Safe Defaults
            Type m_type = HOST_FILE;
            Visibility m_visibility = HIDDEN;

            // Caching of internal data
            std::unique_ptr<std::string> m_cached_ancestor_path = nullptr;
    };

    class RootPathNode : public PathNode {
        public:
            explicit RootPathNode(std::string_view root_name = DEFAULT_ROOT_NAME, PathNode::NodeMap root_nodes = {});
            explicit RootPathNode(std::string_view root_name, std::vector<PathNode> root_nodes);
            explicit RootPathNode(PathNode::NodeMap root_nodes);
            explicit RootPathNode(std::vector<PathNode> root_nodes);

            auto set_name(std::string name) -> PathNode & override;

            auto set_type(Type type) -> PathNode & override;
            auto set_visibility(Visibility visibility) -> PathNode & override;
            auto set_host_path(std::filesystem::path host_path) -> PathNode & override;

            static constexpr const char *DEFAULT_ROOT_NAME = "//fsp";
        private:
            void assert_valid_name(std::string_view name) override;
    };

    class FileMapping {
        public:
            using FilepathSet=std::unordered_set<std::filesystem::path, FileShare::Utils::string_hash, std::equal_to<>>;

            FileMapping() = default;
            FileMapping(RootPathNode root_node, FilepathSet forbidden_paths = FileMapping::default_forbidden_paths());

            [[nodiscard]] auto get_root_name() const -> std::string_view { return m_root_node.get_name(); }
            void set_root_name(std::string root_name) { m_root_node.set_name(std::move(root_name)); }

            // TODO: make sure we can't change root node visibility or type
            [[nodiscard]] auto get_root_node() const -> const RootPathNode & { return m_root_node; }
            [[nodiscard]] auto get_root_node() -> RootPathNode & { return m_root_node; }
            void set_root_node(RootPathNode root_node) { m_root_node = std::move(root_node); }

            [[nodiscard]] auto get_root_nodes() const -> const PathNode::NodeMap & { return m_root_node.get_child_nodes(); }
            void set_root_nodes(PathNode::NodeMap root_nodes) { m_root_node.set_child_nodes(std::move(root_nodes)); }
            void set_root_nodes(std::vector<PathNode> root_nodes);

            [[nodiscard]] auto get_forbidden_paths() const -> const FilepathSet & { return m_forbidden_paths; }
            [[nodiscard]] auto get_forbidden_paths() -> FilepathSet { return m_forbidden_paths; }
            void set_forbidden_paths(FilepathSet paths) { m_forbidden_paths = std::move(paths); }

            auto host_to_virtual(const std::filesystem::path &path) const -> std::filesystem::path;
            auto virtual_to_host(const std::filesystem::path &path) const -> std::filesystem::path;
            static auto virtual_to_host(const std::filesystem::path &virtual_path, const std::optional<PathNode> &node, std::filesystem::path::iterator iter) -> std::filesystem::path;
            auto find_virtual_node(std::filesystem::path virtual_path, std::filesystem::path::iterator &out, bool only_visible = true) const -> std::optional<PathNode>;
            auto find_virtual_node(std::filesystem::path virtual_path, bool only_visible = true) const -> std::optional<PathNode>;

            auto is_forbidden(const std::filesystem::path &path) const -> bool;
        private:
            static auto default_forbidden_paths() -> const FilepathSet &;

            template <class Archive>
            friend void serialize(Archive &archive, FileMapping &file_mapping, std::uint32_t version);

            RootPathNode m_root_node;
            FilepathSet m_forbidden_paths = FileMapping::default_forbidden_paths();
    };
}
