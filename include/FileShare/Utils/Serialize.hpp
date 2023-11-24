/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun May 14 21:46:04 2023 Francois Michaut
** Last update Tue Nov 28 13:41:33 2023 Francois Michaut
**
** Serialize.hpp : Utilities to serialize numbers
*/

#pragma once

#include <cstdint>
#include <string_view>
#include <string>

namespace FileShare::Utils {
    std::string serialize(std::int64_t);
    std::string serialize(std::uint64_t);

    void parse(std::string_view input, std::int64_t &output);
    void parse(std::string_view input, std::uint64_t &output);
}
