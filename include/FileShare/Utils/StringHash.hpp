/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Nov 19 13:31:02 2023 Francois Michaut
** Last update Sun Nov 19 13:31:20 2023 Francois Michaut
**
** StringHash.hpp : Utility hash class for string+string view
*/

#include <string>
#include <filesystem>

namespace FileShare::Utils {
    struct string_hash {
        using hash_type = std::hash<std::string_view>;
        using is_transparent = void;

        std::size_t operator()(const char* str) const        { return hash_type{}(str); }
        std::size_t operator()(std::string_view str) const   { return hash_type{}(str); }
        std::size_t operator()(const std::string &str) const { return hash_type{}(str); }
        std::size_t operator()(const std::filesystem::path &str) const { return hash_type{}(str.string().c_str()); }
    };
}
