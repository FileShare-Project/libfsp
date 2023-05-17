/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun May 14 21:46:04 2023 Francois Michaut
** Last update Wed May 17 08:19:50 2023 Francois Michaut
**
** Serialize.hpp : Utilities to serialize numbers
*/

#pragma once

#include <string>

namespace FileShare::Utils {
    std::string serialize(std::int64_t);
    std::string serialize(std::uint64_t);

    void parse(std::string_view input, std::int64_t &output);
    void parse(std::string_view input, std::uint64_t &output);
}
