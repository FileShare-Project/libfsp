/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Thu Nov 16 22:14:51 2023 Francois Michaut
** Last update Sun Aug 24 11:31:51 2025 Francois Michaut
**
** FileMapping.cpp : Config's PathNode implementation
*/

#include "FileShare/Config/FileMapping.hpp"
#include "FileShare/Utils/Path.hpp"

#include <deque>
#include <memory>
#include <ranges>
#include <sstream>
#include <unordered_set>
#include <utility>

namespace {
     auto stack_to_path(const std::deque<const FileShare::PathNode *> &stack) -> std::string {
        std::stringstream ss;

        if (stack.empty())
            return "";

        for (auto iter = stack.begin(); iter != std::prev(stack.end()); iter++) {
            const auto &node = *iter;
            ss << node->get_name() << "/";
        }
        ss << stack.back()->get_name();
        return ss.str();
    }

    // Removes trailing `/` at the begining and end of the path
    auto trim_node_name(std::string_view name, bool trim_begining = true) -> std::string {
        const char sep = '/'; // TODO: do this properly for windows
        const auto begin = trim_begining ? name.find_first_not_of(sep) : 0;

        if (begin == std::string::npos)
            return ""; // no content

        const auto end = name.find_last_not_of(sep);
        const auto range = end - begin + 1; // end - begin will give us number of caracters between first & last, but we want to include last

        return std::string(name.substr(begin, range));
    }

     auto vector_to_map(std::vector<FileShare::PathNode> nodes) -> FileShare::PathNode::NodeMap {
        FileShare::PathNode::NodeMap result;

        result.reserve(nodes.size());
        for (auto &node : nodes) {
            result.emplace(node.get_name(), std::move(node));
        }
        return result;
    }
}

namespace FileShare {
    PathNode::PathNode(
        std::string name, PathNode *parent, Visibility visibility, NodeMap child_nodes
    ) :
        m_name(std::move(name)), m_parent(parent),
        m_child_nodes(std::move(child_nodes)),
        m_type(Type::VIRTUAL), m_visibility(visibility)
    {
        assert_valid_name(m_name);
        reassign_childs();
    }

    PathNode::PathNode(
        std::string name, PathNode *parent, Type type, std::filesystem::path host_path,
        Visibility visibility
    ) :
        m_name(std::move(name)), m_parent(parent), m_host_path(std::move(host_path)),
        m_type(type), m_visibility(visibility)
    {
        assert_valid_name(m_name);
    }

    PathNode::PathNode(const PathNode &other) :
        m_name(other.m_name), m_parent(other.m_parent),
        m_child_nodes(other.m_child_nodes), m_host_path(other.m_host_path),
        m_type(other.m_type), m_visibility(other.m_visibility)
    {
        reassign_childs();
    }

    auto PathNode::operator=(const PathNode &other) -> PathNode & {
        return *this = PathNode(other);
    }

    auto PathNode::operator==(const PathNode &other) const noexcept -> bool {
        const bool parent_present = m_parent != nullptr;
        const bool other_parent_present = other.m_parent != nullptr;
        const bool same_parent = (parent_present == other_parent_present) && (
            !parent_present || m_parent->get_ancestor_path() == other.m_parent->get_ancestor_path()
        );

        return same_parent && this->m_name == other.m_name &&
            this->m_host_path == other.m_host_path &&
            this->m_type == other.m_type && this->m_visibility == other.m_visibility;
    }

    auto PathNode::make_virtual_node(std::string name, Visibility visibility) -> PathNode {
        return {std::move(name), nullptr, visibility, {}};
    }

    auto PathNode::make_virtual_node(std::string name, Visibility visibility, NodeMap child_nodes) -> PathNode {
        return {std::move(name), nullptr, visibility, std::move(child_nodes)};
    }

    auto PathNode::make_virtual_node(std::string name, Visibility visibility, std::vector<PathNode> child_nodes) -> PathNode {
        return {std::move(name), nullptr, visibility, vector_to_map(std::move(child_nodes))};
    }

    auto PathNode::make_virtual_node(std::string name, NodeMap child_nodes, Visibility visibility) -> PathNode {
        return {std::move(name), nullptr, visibility, std::move(child_nodes)};
    }

    auto PathNode::make_virtual_node(std::string name, std::vector<PathNode> child_nodes, Visibility visibility) -> PathNode {
        return make_virtual_node(std::move(name), visibility, std::move(child_nodes));
    }

    auto PathNode::make_host_node(std::string name, Type type, std::filesystem::path host_path, Visibility visibility) -> PathNode {
        assert_not_virtual_type(type);
        return {std::move(name), nullptr, type, std::move(host_path), visibility};
    }

    auto PathNode::add_child_node(PathNode node) -> PathNode & {
        (void)insert_child_node(std::move(node));
        return *this;
    }

    auto PathNode::insert_child_node(PathNode node) -> PathNode & {
        assert_virtual_type(m_type);
        node.assign_parent(this);

        std::pair<NodeMap::iterator, bool> result = m_child_nodes.emplace(
            node.get_name(), std::move(node)
        );

        if (!result.second) {
            throw std::runtime_error("A node already exists with that name");
        }
        return result.first->second;
    }

    auto PathNode::remove_child_node(const std::string &name) -> PathNode & {
        m_child_nodes.erase(name);
        return *this;
    }

    auto PathNode::clear_child_nodes() -> PathNode & {
        m_child_nodes.clear();
        return *this;
    }

    auto PathNode::set_child_nodes(NodeMap nodes) -> PathNode & {
        if (!nodes.empty()) {
            assert_virtual_type(m_type);
        }
        m_child_nodes = std::move(nodes);

        reassign_childs();
        return *this;
    }

    auto PathNode::set_child_nodes(std::vector<PathNode> nodes) -> PathNode & {
        return set_child_nodes(vector_to_map(std::move(nodes)));
    }

    auto PathNode::set_name(std::string name) -> PathNode & {
        assert_valid_name(name);
        m_name = std::move(name);
        return *this;
    }

    auto PathNode::set_type(Type type) -> PathNode & {
        if (!m_child_nodes.empty()) {
            assert_virtual_type(type);
        } else if (!m_host_path.empty()) {
            assert_not_virtual_type(type);
        }
        m_type = type;
        return *this;
    }

    auto PathNode::set_host_path(std::filesystem::path host_path) -> PathNode & {
        if (!host_path.empty()) {
            assert_not_virtual_type(m_type);
        }
        m_host_path = std::move(host_path);
        return *this;
    }

    auto PathNode::get_ancestor_path() const -> std::filesystem::path {
        if (m_cached_ancestor_path) {
            return *m_cached_ancestor_path;
        }

        std::deque<const PathNode *> stack;
        const PathNode *current_node = m_parent;

        while (current_node) {
            stack.push_back(current_node);
            current_node = current_node->m_parent;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
        auto *mutable_this = const_cast<PathNode *>(this);
        mutable_this->m_cached_ancestor_path = std::make_unique<std::string>(stack_to_path(stack));

        return *m_cached_ancestor_path;
    }

    void PathNode::assert_virtual_type(Type type) {
        if (type != Type::VIRTUAL) {
            throw std::runtime_error("Only VIRTUAL PathNode can have child_nodes.");
        }
    }

    void PathNode::assert_not_virtual_type(Type type) {
        if (type == Type::VIRTUAL) {
            throw std::runtime_error("VIRTUAL PathNode cannot have a host_path.");
        }
    }

    void PathNode::assert_valid_name(std::string_view name) {
        // TODO: do this properly for windows
        if (name.find('/') != std::string::npos) {
            throw std::runtime_error("PathNode name cannot contain '/'");
        }
        if (name.empty()) {
            throw std::runtime_error("PathNode name cannot be empty");
        }
    }

    void PathNode::assign_parent(PathNode *other) {
        m_parent = other;
        m_cached_ancestor_path = nullptr;
    }

    void PathNode::reassign_childs() {
        for (auto &node : m_child_nodes) {
            node.second.assign_parent(this);
        }
    }

    RootPathNode::RootPathNode(std::string_view root_name, PathNode::NodeMap root_nodes) :
        PathNode(trim_node_name(root_name), nullptr, Visibility::VISIBLE, std::move(root_nodes))
    {
        set_name(trim_node_name(root_name, false)); // We want to keep the leading `/` if any
        assert_valid_name(get_name());
    }

    RootPathNode::RootPathNode(std::string_view root_name, std::vector<PathNode> root_nodes) :
        RootPathNode(root_name, vector_to_map(std::move(root_nodes)))
    {}

    RootPathNode::RootPathNode(PathNode::NodeMap root_nodes) :
        RootPathNode(DEFAULT_ROOT_NAME, std::move(root_nodes))
    {}

    RootPathNode::RootPathNode(std::vector<PathNode> root_nodes) :
        RootPathNode(DEFAULT_ROOT_NAME, std::move(root_nodes))
    {}

    auto RootPathNode::set_name(std::string name) -> PathNode & {
        return PathNode::set_name(trim_node_name(name, false));
    }

    auto RootPathNode::set_type(Type type) -> PathNode & {
        if (type != Type::VIRTUAL) {
            throw std::runtime_error("RootPathNode type must always be VIRTUAL.");
        }
        return *this;
    }

    auto RootPathNode::set_visibility(Visibility visibility) -> PathNode & {
        if (visibility != Visibility::VISIBLE) {
            throw std::runtime_error("RootPathNode visibility must always be VISIBLE.");
        }
        return *this;
    }

    auto RootPathNode::set_host_path(std::filesystem::path host_path) -> PathNode & {
        if (!host_path.empty()) {
            throw std::runtime_error("RootPathNode cannot have a host_path.");
        }
        return *this;
    }

    void RootPathNode::assert_valid_name(std::string_view name) {
        if (name.empty()) {
            throw std::runtime_error("RootPathNode name cannot be empty");
        }
        // TODO: do this properly for windows
        auto last_slash = name.find_last_of('/');
        auto first_char = name.find_first_not_of('/');

        if (first_char == std::string::npos) {
            throw std::runtime_error("RootPathNode name cannot be only made of directory separators");
        }
        if (first_char < last_slash) {
            throw std::runtime_error("RootPathNode name can only contain directory separators at the begining");
        }
        if (first_char > 2) {
            throw std::runtime_error("RootPathNode name can only contain up to 2 directory separators at the begining");
        }
    }

    FileMapping::FileMapping(RootPathNode root_node, FilepathSet forbidden_paths) :
        m_root_node(std::move(root_node)), m_forbidden_paths(std::move(forbidden_paths))
    {}

    void FileMapping::set_root_nodes(std::vector<PathNode> root_nodes) {
        m_root_node.set_child_nodes(vector_to_map(std::move(root_nodes)));
    }

    auto FileMapping::default_forbidden_paths() -> const FileMapping::FilepathSet & {
        // TODO: find all paths that should be forbidden
        static FilepathSet forbidden_paths = [](){
            auto vector = FileShare::Utils::resolve_home_components({
                "~/.ssh", "~/.fsp", "/etc/passwd", "/root"
            });

            return FilepathSet{vector.begin(), vector.end()};
        }();

        return forbidden_paths;
    }

    auto FileMapping::host_to_virtual(const std::filesystem::path &path) const -> std::filesystem::path {
        std::deque<const PathNode *> stack;
        std::unordered_set<const PathNode *> visited_set;
        const PathNode *current_node = &m_root_node;

        // TODO: make a better algo, way too many branches in this one
        stack.push_back(current_node);
        visited_set.insert(current_node);

        while (current_node) {
            if (current_node->get_visibility() == PathNode::HIDDEN) {
                stack.pop_back();
                current_node = stack.empty() ? nullptr : stack.back();
                continue;
            }

            if (current_node->get_type() == PathNode::VIRTUAL) {
                bool found_new_node = false;

                for (const auto &[_, node] : current_node->get_child_nodes()) {
                    if (node.get_visibility() != PathNode::HIDDEN && !visited_set.contains(&node)) {
                        current_node = &node;
                        visited_set.insert(current_node);
                        stack.push_back(current_node);
                        found_new_node = true;
                        break;
                    }
                }
                if (found_new_node) {
                    continue; // This is what gives the "depth-first" behavior.
                }
            } else if (current_node->get_host_path() == path) {
                return stack_to_path(stack);
            } else if (current_node->get_type() == PathNode::HOST_FOLDER) {
                auto iter = Utils::find_folder_in_path(path, current_node->get_host_path());

                if (iter != path.end()) {
                    std::filesystem::path result = stack_to_path(stack);

                    for (; iter != path.end(); iter++) {
                        result /= *iter;
                    }
                    return result;
                }
            }
            stack.pop_back();
            current_node = stack.empty() ? nullptr : stack.back();
        }
        return ""; // Couldn't find a match
        // NOTE: When Client uses this to translate a path to send, client should use path.filename() without root_name()
        // This way, peer knows that this file is "temporary" and it cannot request it normally
    }

    auto FileMapping::virtual_to_host(const std::filesystem::path &virtual_path) const -> std::filesystem::path {
        std::filesystem::path::iterator iter;
        auto node = find_virtual_node(virtual_path, iter);

        return virtual_to_host(virtual_path, node, iter);
    }

    auto FileMapping::virtual_to_host(const std::filesystem::path &virtual_path, const std::optional<PathNode> &node, std::filesystem::path::iterator iter) -> std::filesystem::path {
        if (!node.has_value()) {
            return "";
        }
        if (node->get_type() == PathNode::HOST_FOLDER) {
            std::filesystem::path result = node->get_host_path();

            for (; iter != virtual_path.end(); iter++) {
                result /= *iter;
            }
            return result;
        }
        return node->get_host_path(); // Virtual nodes will return ""
    }

    auto FileMapping::find_virtual_node(std::filesystem::path virtual_path, std::filesystem::path::iterator &out, bool only_visible) const -> std::optional<PathNode> {
        std::string str = trim_node_name(virtual_path.string());
        const PathNode *result = &m_root_node;
        // TODO: Instead of trimming, do smth else, cause right now `/fsp/aaa` will be considered a virtual path to /aaa -> What if /fsp is an actual host folder ??
        const auto &root_name = trim_node_name(m_root_node.get_name());

        // Root name is optional in the path
        if (str.starts_with(root_name)) {
            virtual_path = str.substr(root_name.size());
        }
        for (out = virtual_path.begin(); out != virtual_path.end(); out++) {
            if (*out == "/")
                continue;

            const auto &child_nodes = result->get_child_nodes();
            auto node = child_nodes.find(*out);
            // TODO: Why do we have only_visible param ? When would that not be true ?
            bool can_see_node = node != child_nodes.end() && (!only_visible || node->second.get_visibility() == PathNode::VISIBLE);

            if (can_see_node) {
                result = &node->second;

                if (result->get_type() == PathNode::HOST_FOLDER) {
                    out++; // return past the node iterator
                    return *result;
                }
            } else {
                return {};
            }
        }
        return *result; // TODO: Doesn't this slice the root_node if it gets returned ?
    }

    auto FileMapping::find_virtual_node(std::filesystem::path virtual_path, bool only_visible) const -> std::optional<PathNode> {
        std::filesystem::path::iterator iter;

        return FileMapping::find_virtual_node(std::move(virtual_path), iter, only_visible);
    }

    auto FileMapping::is_forbidden(const std::filesystem::path &path) const -> bool {
        if (m_forbidden_paths.contains(path))
            return true;
        std::string path_str = path.generic_string();

        for (const auto &iter : m_forbidden_paths) {
            std::string iter_str = iter.generic_string();

            if (iter_str.ends_with('/')) {
                iter_str.pop_back();
            }

            if (path_str.starts_with(iter_str) && (iter_str.size() == path_str.size() || path_str[iter_str.size()] == '/')) {
                return true;
            }
        }
        return false;
    }
}
