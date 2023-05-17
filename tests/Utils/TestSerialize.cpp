/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Wed May 17 08:41:29 2023 Francois Michaut
** Last update Wed May 17 09:04:05 2023 Francois Michaut
**
** TestSerialize.cpp : Testing of utilities to serialize numbers
*/

#include "FileShare/Utils/Serialize.hpp"
#include "FileShare/Utils/VarInt.hpp"

#include <cassert>

using namespace FileShare::Utils;

void test_back_forth_serialize() {
    std::string str;
    std::uint64_t out;

    for (std::uint64_t i = 0; i < 0xFFFF; i++) {
        str = serialize(i);
        parse(str, out);

        assert(out == i);
    }

    // Skipping a few...

    for (std::uint64_t i = 0xFFFFFFFFFFFF0000; i < 0xFFFFFFFFFFFFFFFF; i++) {
        str = serialize(i);
        parse(str, out);

        assert(out == i);
    }
}

void test_back_forth_signed_serialize() {
    std::string str;
    std::int64_t out;

    for (std::int64_t i = 0; i < 0xFFFF; i++) {
        str = serialize(i);
        parse(str, out);

        assert(out == i);
    }

    // Skipping a few...

    for (std::int64_t i = 0xFFFFFFFFFFFF0000; i < 0xFFFFFFFFFFFFFFFF; i++) {
        str = serialize(i);
        parse(str, out);

        assert(out == i);
    }
}

int Utils_TestSerialize(int, char**)
{
    test_back_forth_serialize();
    test_back_forth_signed_serialize();
    return 0;
}
