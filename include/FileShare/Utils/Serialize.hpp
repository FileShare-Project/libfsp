/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun May 14 21:46:04 2023 Francois Michaut
** Last update Fri Aug 15 20:46:49 2025 Francois Michaut
**
** Serialize.hpp : Utilities to serialize numbers
*/

#pragma once

#include <cstdint>
#include <string_view>
#include <string>

namespace FileShare::Utils {
    auto serialize(std::int64_t) -> std::string;
    auto serialize(std::uint64_t) -> std::string;

    void parse(std::string_view input, std::int64_t &output);
    void parse(std::string_view input, std::uint64_t &output);
}
