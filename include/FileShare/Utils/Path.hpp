/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Oct 13 18:59:40 2022 Francois Michaut
** Last update Wed Aug  6 16:14:08 2025 Francois Michaut
**
** Path.hpp : Utilities to manpulate paths in a cross plateform way
*/

#pragma once

#include <filesystem>
#include <vector>

namespace FileShare::Utils {
    auto home_directoy() -> std::filesystem::path;
    auto home_directoy(const std::string &user) -> std::filesystem::path;
    auto resolve_home_component(const std::filesystem::path &path) -> std::filesystem::path;
    auto resolve_home_components(const std::vector<std::filesystem::path> &paths) -> std::vector<std::filesystem::path>;

    auto path_contains_folder(const std::filesystem::path &path, const std::filesystem::path &folder) -> bool;
    auto find_folder_in_path(const std::filesystem::path &path, const std::filesystem::path &folder) -> std::filesystem::path::iterator;

    auto make_temporary_file(const std::string &basename) -> std::pair<int, std::string>;
}
