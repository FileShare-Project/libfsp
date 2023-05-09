/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Oct 13 19:09:01 2022 Francois Michaut
** Last update Tue May  9 09:48:02 2023 Francois Michaut
**
** Path.cpp : Implementation of utilities to manpulate paths in a cross plateform way
*/

#include "FileShare/Utils/Path.hpp"

#ifdef _WIN32
#else
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace FileShare::Utils {
    std::filesystem::path home_directoy(const std::string &user) {
        std::string res;
#ifdef _WIN32
        // https://github.com/python/cpython/blob/main/Lib/ntpath.py
#else
        char *home = std::getenv("HOME");
        struct passwd *entry = nullptr;

        if (home == nullptr) {
            entry = user.empty() ? getpwuid(geteuid()) : getpwnam(user.c_str());
            home = entry != nullptr ? entry->pw_dir : nullptr;
        }
        res = home;
        if (res.back() != '/')
            res += '/';
#endif
        return res;
    }

    std::filesystem::path resolve_home_component(const std::filesystem::path &path) {
        using value_type=std::filesystem::path::value_type;
        using string_type=std::filesystem::path::string_type;
        string_type str = path.generic_string<value_type>();

        if (str.find('~') != 0)
            return path;
        auto pos = str.find('/');
        auto home = home_directoy(str.substr(1, (pos == std::string::npos ? pos : pos - 1)));

        if (home.empty())
            return path;
        if (pos == std::string::npos)
            return home;
        return home / str.substr(pos + 1);
    }

    std::vector<std::filesystem::path> resolve_home_components(const std::vector<std::filesystem::path> &paths) {
        std::vector<std::filesystem::path> result;

        result.reserve(paths.size());
        for (auto &path : paths) {
            result.emplace_back(resolve_home_component(path));
        }
        return result;
    }
}
