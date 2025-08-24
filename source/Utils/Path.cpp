/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Oct 13 19:09:01 2022 Francois Michaut
** Last update Wed Aug  6 22:32:34 2025 Francois Michaut
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
    auto home_directoy(const std::string &user) -> std::filesystem::path {
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
        // TODO: Make Threadsafe + Cache env + passwd entry
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

    auto home_directoy() -> std::filesystem::path {
        static std::filesystem::path home = home_directoy("");

        return home;
    }

    auto resolve_home_component(const std::filesystem::path &path) -> std::filesystem::path {
        auto str = path.generic_string();

        if (!str.starts_with('~'))
            return path;
        auto pos = str.find('/');
        auto home = pos == 1 ? home_directoy() : home_directoy(str.substr(1, (pos == std::string::npos ? pos : pos - 1)));

        if (home.empty())
            return path;
        if (pos == std::string::npos)
            return home;
        return home / str.substr(pos + 1);
    }

    auto resolve_home_components(const std::vector<std::filesystem::path> &paths) -> std::vector<std::filesystem::path> {
        std::vector<std::filesystem::path> result;

        result.reserve(paths.size());
        for (const auto &path : paths) {
            result.emplace_back(resolve_home_component(path));
        }
        return result;
    }

    auto path_contains_folder(const std::filesystem::path &path, const std::filesystem::path &folder) -> bool {
        return find_folder_in_path(path, folder) != path.end();
    }

    auto find_folder_in_path(const std::filesystem::path &path, const std::filesystem::path &folder) -> std::filesystem::path::iterator {
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
