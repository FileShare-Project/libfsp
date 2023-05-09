/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 17:23:39 2023 Francois Michaut
** Last update Sun May 14 19:58:25 2023 Francois Michaut
**
** FileHash.hpp : Function to hash file contents
*/

#pragma once

#include <filesystem>
#include <string>

namespace FileShare::Utils {
    // TODO: add more
    enum class HashAlgorithm {
        MD5     = 0x01,
        SHA256  = 0x02,
        SHA512  = 0x03
    };

    const char *algo_to_string(HashAlgorithm algo);
    std::size_t algo_hash_size(HashAlgorithm algo);
    std::string file_hash(HashAlgorithm algo, const std::filesystem::path &path);
}
