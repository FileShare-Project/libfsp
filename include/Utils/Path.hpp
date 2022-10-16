/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Oct 13 18:59:40 2022 Francois Michaut
** Last update Mon Oct 24 19:06:27 2022 Francois Michaut
**
** Path.hpp : Utilities to manpulate paths in a cross plateform way
*/

#pragma once

#include <filesystem>
#include <vector>

namespace Utils {
    std::filesystem::path home_directoy(const std::string &user = "");
    std::filesystem::path resolve_home_component(const std::filesystem::path &path);
    std::vector<std::filesystem::path> resolve_home_components(const std::vector<std::filesystem::path> &paths);
}
