/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Oct 13 19:09:01 2022 Francois Michaut
** Last update Thu Nov 23 09:05:00 2023 Francois Michaut
**
** Path.cpp : Implementation of utilities to manpulate paths in a cross plateform way
*/

#include "FileShare/Utils/Path.hpp"

#include <CppSockets/OSDetection.hpp>

#ifdef OS_WINDOWS
  #include <userenv.h>

  #include <array>
  #include <string>
#else
  #include <pwd.h>
  #include <sys/types.h>
  #include <unistd.h>
#endif

namespace FileShare::Utils {
    std::filesystem::path home_directoy(const std::string &user) {
        std::string res;
        char *home = nullptr;

#ifdef OS_WINDOWS
        if (user.empty()) {
            home = std::getenv("USERPROFILE");

            if (home != nullptr) {
                res = home;
            } else {
                char *drive = std::getenv("HOMEDRIVE");
                std::filesystem::path homepath = drive ? drive : "";

                home = std::getenv("HOMEPATH");
                if (home != nullptr) {
                    res = (homepath / home).string();
                }
            }
        }
        if (res.empty()) {
            std::array<char, 4096> users_path = {0};
            DWORD size = users_path.size();

            if (GetProfilesDirectoryA(users_path.data(), &size)) {
                std::filesystem::path homepath = users_path.data();

                res = (homepath / user).string();
            }
        }
#else
        if (user.empty()) {
            home = std::getenv("HOME");
        }
        if (home == nullptr) {
            struct passwd *entry = user.empty() ? getpwuid(geteuid()) : getpwnam(user.c_str());

            home = entry != nullptr ? entry->pw_dir : nullptr;
        }
        if (home == nullptr) {
            return "";
        }
        res = home;
#endif
        if (res.empty()) {
            return res;
        }
        if (res.back() != std::filesystem::path::preferred_separator)
            res += std::filesystem::path::preferred_separator;
        return res;
    }

    std::filesystem::path home_directoy() {
        static std::filesystem::path home = home_directoy("");

        return home;
    }

    std::filesystem::path resolve_home_component(const std::filesystem::path &path) {
        auto str = path.generic_string();

        if (str.find('~') != 0)
            return path;
        auto pos = str.find('/');
        auto home = pos == 1 ? home_directoy() : home_directoy(str.substr(1, (pos == std::string::npos ? pos : pos - 1)));

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

    bool path_contains_folder(const std::filesystem::path &path, const std::filesystem::path &folder) {
        return find_folder_in_path(path, folder) != path.end();
    }

    std::filesystem::path::iterator find_folder_in_path(const std::filesystem::path &path, const std::filesystem::path &folder) {
        auto path_iter = path.begin();
        auto folder_iter = folder.begin();

        while (path_iter != path.end() && folder_iter != folder.end()) {
            if (*path_iter != *folder_iter) {
                return path.end();
            }
            path_iter++;
            folder_iter++;
        }
        return path_iter;
    }
}
