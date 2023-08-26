/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Wed May 17 08:06:48 2023 Francois Michaut
** Last update Sat Aug 26 18:43:57 2023 Francois Michaut
**
** Serialize.cpp : Implementation of utilities to serialize numbers
*/

#include "FileShare/Utils/Serialize.hpp"

#include <stdexcept>

namespace FileShare::Utils {
    std::string serialize(std::int64_t value) {
        return serialize((std::uint64_t)value);
    }

    std::string serialize(std::uint64_t value) {
        std::string str;

        str.reserve(8);
        for (std::uint8_t i = 0; i < 8; i++) {
            str += value & 0xFF;
            value >>= 8;
        }
        return str;
    }

    void parse(std::string_view input, std::int64_t &output) {
        std::uint64_t result = 0;

        parse(input, result);
        output = result;
    }

    void parse(std::string_view input, std::uint64_t &output) {
        std::uint64_t result = 0;

        if (input.size() > 8)
            throw std::runtime_error("Input can't be larger than the output byte size");
        for (auto c = input.rbegin(); c != input.rend(); c++) {
            result = (result << 8) + (std::uint8_t)*c;
        }
        output = result;
    }
}
