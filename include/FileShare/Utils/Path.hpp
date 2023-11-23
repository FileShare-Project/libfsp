/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Oct 13 18:59:40 2022 Francois Michaut
** Last update Thu Nov 23 08:56:46 2023 Francois Michaut
**
** Path.hpp : Utilities to manpulate paths in a cross plateform way
*/

#pragma once

#include <filesystem>
#include <vector>

namespace FileShare::Utils {
    std::filesystem::path home_directoy();
    std::filesystem::path home_directoy(const std::string &user);
    std::filesystem::path resolve_home_component(const std::filesystem::path &path);
    std::vector<std::filesystem::path> resolve_home_components(const std::vector<std::filesystem::path> &paths);

    bool path_contains_folder(const std::filesystem::path &path, const std::filesystem::path &folder);
    std::filesystem::path::iterator find_folder_in_path(const std::filesystem::path &path, const std::filesystem::path &folder);
}
