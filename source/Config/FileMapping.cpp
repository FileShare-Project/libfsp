/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Thu Nov 16 22:14:51 2023 Francois Michaut
** Last update Sat Aug 10 09:31:18 2024 Francois Michaut
**
** FileMapping.cpp : Config's PathNode implementation
*/

#include "FileShare/Config.hpp"
#include "FileShare/Utils/Path.hpp"

#include <algorithm>
#include <deque>
#include <unordered_set>
#include <ranges>
#include <sstream>

static std::string stack_to_path(const std::deque<const FileShare::PathNode *> &stack) {
    std::stringstream ss;

    if (stack.empty())
        return "";

    for (auto iter = stack.begin(); iter != std::prev(stack.end()); iter++) {
        const auto &node = *iter;
        ss << node->get_name() << "/";
    }
    if (stack.size() > 0) {
        ss << stack.back()->get_name();
    }
    return ss.str();
}

static std::string trim_node_name(std::string_view name) {
    const char sep = '/'; // TODO: do this properly for windows
    const auto begin = name.find_first_not_of(sep);

    if (begin == std::string::npos)
        return ""; // no content

    const auto end = name.find_last_not_of(sep);
    const auto range = end - begin + 1; // end - begin will give us number of caracters between first & last, but we want to include last

    return std::string(name.substr(begin, range));
}

static FileShare::PathNode::NodeMap vector_to_map(std::vector<FileShare::PathNode> nodes) {
    FileShare::PathNode::NodeMap result;

    result.reserve(nodes.size());
    for (const auto &node : nodes) {
        result.emplace(node.get_name(), std::move(node));
    }
    return result;
}

namespace FileShare {
    PathNode::PathNode(std::string name, Visibility visibility, NodeMap child_nodes) :
        m_name(std::move(name)), m_type(VIRTUAL), m_visibility(visibility), m_child_nodes(std::move(child_nodes))
    {
        assert_valid_name(m_name);
    }

    PathNode::PathNode(std::string name, Type type, std::filesystem::path host_path, Visibility visibility) :
        m_name(std::move(name)), m_type(type), m_visibility(visibility), m_host_path(std::move(host_path))
    {
        assert_valid_name(m_name);
    }

    bool PathNode::operator==(const PathNode &other) const noexcept {
        return this->get_name() == other.get_name() && this->get_type() == other.get_type() &&
            this->get_visibility() == other.get_visibility() && this->get_child_nodes() == other.get_child_nodes() &&
            this->get_host_path() == other.get_host_path();
    }

    PathNode PathNode::make_virtual_node(std::string name, Visibility visibility) {
        return PathNode(std::move(name), visibility, {});
    }

    PathNode PathNode::make_virtual_node(std::string name, Visibility visibility, NodeMap child_nodes) {
        return PathNode(std::move(name), visibility, std::move(child_nodes));
    }

    PathNode PathNode::make_virtual_node(std::string name, Visibility visibility, std::vector<PathNode> child_nodes) {
        return PathNode(std::move(name), visibility, vector_to_map(std::move(child_nodes)));
    }

    PathNode PathNode::make_virtual_node(std::string name, NodeMap child_nodes, Visibility visibility) {
        return PathNode(std::move(name), visibility, std::move(child_nodes));
    }

    PathNode PathNode::make_virtual_node(std::string name, std::vector<PathNode> child_nodes, Visibility visibility) {
        return make_virtual_node(std::move(name), visibility, std::move(child_nodes));
    }

    PathNode PathNode::make_host_node(std::string name, Type type, std::filesystem::path host_path, Visibility visibility) {
        assert_not_virtual_type(type);
        return PathNode(std::move(name), type, std::move(host_path), visibility);
    }

    PathNode &PathNode::add_child_node(PathNode node) {
        assert_virtual_type(m_type);
        auto result = m_child_nodes.emplace(node.get_name(), std::move(node));

        if (!result.second) {
            throw std::runtime_error("A node already exists with that name");
        }
        return *this;
    }

    PathNode &PathNode::remove_child_node(std::string_view name) {
        std::erase_if(m_child_nodes, [&name](const auto &iter) { return iter.first == name;});
        return *this;
    }

    PathNode &PathNode::clear_child_nodes() {
        m_child_nodes.clear();
        return *this;
    }

    PathNode::NodeMap &PathNode::get_child_nodes() { return m_child_nodes; }
    const PathNode::NodeMap &PathNode::get_child_nodes() const { return m_child_nodes; }
    PathNode &PathNode::set_child_nodes(NodeMap nodes) {
        if (!nodes.empty()) {
            assert_virtual_type(m_type);
        }
        m_child_nodes = std::move(nodes);
        return *this;
    }
    PathNode &PathNode::set_child_nodes(std::vector<PathNode> nodes) {
        assert_virtual_type(m_type);
        m_child_nodes = vector_to_map(std::move(nodes));
        return *this;
    }

    std::string_view PathNode::get_name() const { return m_name; }
    const std::string &PathNode::get_name_str() const { return m_name; }
    PathNode &PathNode::set_name(std::string name) {
        assert_valid_name(name);
        m_name = std::move(name);
        return *this;
    }

    bool PathNode::is_host() const { return !is_virtual(); }
    bool PathNode::is_virtual() const { return m_type == VIRTUAL; }
    bool PathNode::is_host_file() const { return m_type == HOST_FILE; }
    bool PathNode::is_host_folder() const { return m_type == HOST_FOLDER; }

    PathNode::Type PathNode::get_type() const { return m_type; }
    PathNode &PathNode::set_type(Type type) {
        if (!m_child_nodes.empty()) {
            assert_virtual_type(type);
        } else if (!m_host_path.empty()) {
            assert_not_virtual_type(type);
        }
        m_type = type;
        return *this;
    }

    PathNode::Visibility PathNode::get_visibility() const { return m_visibility; }
    PathNode &PathNode::set_visibility(Visibility visibility) {
        m_visibility = visibility;
        return *this;
    }

    const std::filesystem::path &PathNode::get_host_path() const { return m_host_path; }
    PathNode &PathNode::set_host_path(std::filesystem::path host_path) {
        if (!host_path.empty()) {
            assert_not_virtual_type(m_type);
        }
        m_host_path = std::move(host_path);
        return *this;
    }

    void PathNode::assert_virtual_type(Type type) {
        if (type != VIRTUAL) {
            throw std::runtime_error("Only VIRTUAL PathNode can have child_nodes.");
        }
    }

    void PathNode::assert_not_virtual_type(Type type) {
        if (type == VIRTUAL) {
            throw std::runtime_error("VIRTUAL PathNode cannot have a host_path.");
        }
    }

    void PathNode::assert_valid_name(std::string_view name) {
        // TODO: do this properly for windows
        if (name.find('/') != std::string::npos) {
            throw std::runtime_error("PathNode name cannot contain '/'");
        } else if (name.empty()) {
            throw std::runtime_error("PathNode name cannot be empty");
        }
    }

    RootPathNode::RootPathNode(std::string root_name, PathNode::NodeMap root_nodes) :
        PathNode(trim_node_name(root_name), Visibility::VISIBLE, std::move(root_nodes))
    {
        set_name(root_name);
    }

    RootPathNode::RootPathNode(std::string root_name, std::vector<PathNode> root_nodes) :
        PathNode(trim_node_name(root_name), Visibility::VISIBLE, vector_to_map(std::move(root_nodes)))
    {
        set_name(root_name);
    }

    RootPathNode::RootPathNode(PathNode::NodeMap root_nodes) :
        RootPathNode(default_root_name, std::move(root_nodes))
    {}

    RootPathNode::RootPathNode(std::vector<PathNode> root_nodes) :
        RootPathNode(default_root_name, std::move(root_nodes))
    {}

    void RootPathNode::assert_valid_name(std::string_view name) {
        if (name.empty()) {
            throw std::runtime_error("PathNode name cannot be empty");
        }
        // TODO: do this properly for windows
        auto last_slash = name.find_last_of('/');
        auto first_char = name.find_first_not_of('/');

        if (first_char == std::string::npos) {
            throw std::runtime_error("RootPathNode name cannot be only made of directory separators");
        } else if (first_char < last_slash) {
            throw std::runtime_error("RootPathNode name can only contain directory separators at the begining");
        }
    }

    FileMapping::FileMapping(RootPathNode root_node, FilepathSet forbidden_paths) :
        m_root_node(std::move(root_node)), m_forbidden_paths(std::move(forbidden_paths))
    {}

    std::string_view FileMapping::get_root_name() const { return m_root_node.get_name(); }
    void FileMapping::set_root_name(std::string root_name) { m_root_node.set_name(std::move(root_name)); }

    const RootPathNode &FileMapping::get_root_node() const { return m_root_node; }
    RootPathNode &FileMapping::get_root_node() { return m_root_node; }
    void FileMapping::set_root_node(RootPathNode root_node) { m_root_node = std::move(root_node); }

    const PathNode::NodeMap &FileMapping::get_root_nodes() const { return m_root_node.get_child_nodes(); }
    PathNode::NodeMap &FileMapping::get_root_nodes() { return m_root_node.get_child_nodes(); }
    void FileMapping::set_root_nodes(PathNode::NodeMap root_nodes) { m_root_node.set_child_nodes(std::move(root_nodes)); }
    void FileMapping::set_root_nodes(std::vector<PathNode> root_nodes) { m_root_node.set_child_nodes(vector_to_map(std::move(root_nodes))); }

    const FileMapping::FilepathSet &FileMapping::get_forbidden_paths() const { return m_forbidden_paths; }
    FileMapping::FilepathSet FileMapping::get_forbidden_paths() { return m_forbidden_paths; }
    void FileMapping::set_forbidden_paths(FileMapping::FilepathSet paths) { m_forbidden_paths = std::move(paths); }

    const FileMapping::FilepathSet &FileMapping::default_forbidden_paths() {
        // TODO: find all paths that should be forbidden
        static FilepathSet forbidden_paths = [](){
            auto vector = FileShare::Utils::resolve_home_components({"~/.ssh", "~/.fsp", "/etc/passwd", "/root"});

            return FilepathSet{vector.begin(), vector.end()};
        }();

        return forbidden_paths;
    }

    std::filesystem::path FileMapping::host_to_virtual(const std::filesystem::path &path) const {
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

    std::filesystem::path FileMapping::virtual_to_host(const std::filesystem::path &virtual_path) const {
        std::filesystem::path::iterator iter;
        auto node = find_virtual_node(virtual_path, iter);

        return virtual_to_host(virtual_path, node, iter);
    }

    std::filesystem::path FileMapping::virtual_to_host(const std::filesystem::path &virtual_path, const std::optional<PathNode> &node, std::filesystem::path::iterator iter) const {
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

    std::optional<PathNode> FileMapping::find_virtual_node(std::filesystem::path virtual_path, std::filesystem::path::iterator &iter, bool only_visible) const {
        std::string str = trim_node_name(virtual_path.string());
        const PathNode *result = &m_root_node;
        const auto &root_name = trim_node_name(m_root_node.get_name());

        // Root name is optional in the path
        if (str.starts_with(root_name)) {
            virtual_path = str.substr(root_name.size());
        }
        for (iter = virtual_path.begin(); iter != virtual_path.end(); iter++) {
            if (*iter == "/")
                continue;

            auto &child_nodes = result->get_child_nodes();
            auto node = child_nodes.find(*iter);
            bool can_see_node = node != child_nodes.end() && (!only_visible || node->second.get_visibility() == PathNode::VISIBLE);

            if (can_see_node) {
                result = &node->second;

                if (result->get_type() == PathNode::HOST_FOLDER) {
                    iter++; // return past the node iterator
                    return *result;
                }
            } else {
                return {};
            }
        }
        return *result;
    }

    std::optional<PathNode> FileMapping::find_virtual_node(std::filesystem::path virtual_path, bool only_visible) const {
        std::filesystem::path::iterator iter;

        return FileMapping::find_virtual_node(virtual_path, iter, only_visible);
    }

    bool FileMapping::is_forbidden(const std::filesystem::path &path) const {
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
