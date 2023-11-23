/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 16:23:13 2023 Francois Michaut
** Last update Wed Nov 22 20:26:16 2023 Francois Michaut
**
** TestVarInt.cpp : Variable size integer tests
*/

#include "FileShare/Utils/VarInt.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <limits>
#include <map>
#include <random>

using namespace FileShare::Utils;

static std::map<std::size_t, std::string> known_varints_map = {
    {0, std::string("\x00", 1)},
    {128, "\x80\x01"},
    {200, "\xC8\x01"},
    {256, "\x80\x02"},
    {831, "\xBF\x06"},
    {1000, "\xE8\x07"},
    {12345, "\xB9\x60"},
    {123456, "\xC0\xC4\x07"},
    {184467440737, "\xE1\x88\xF7\x98\xAF\x05"},
    {std::numeric_limits<std::size_t>::max(), "\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\xFF\x01"}
};

static void test_varints_7bits() {
    VarInt v;

    for (int i = 0; i < 0x80; i++) {
        std::string_view str;

        v.reset(i);
        str = v.to_string();

        assert(str.size() == 1);
        assert(str[0] == i);
        assert(v.to_number() == i);
        assert(v.byte_size() == 1);
    }
}

static void test_varints_after_8bits() {
    VarInt v;

    for (std::size_t i = 128; i < 0x4000; i++) {
        std::string_view str;

        v.reset(i);
        str = v.to_string();

        assert(str.size() == 2);
        assert(v.to_number() == i);
        assert((unsigned char)str[0] == ((i & 0x7F) | 0x80));
        assert((unsigned char)str[1] == (i & 0x3F80) >> 7);
    }
}

static void test_known_varints() {
    VarInt varint;

    for (auto const& [number, string] : known_varints_map) {
        varint.reset(number);

        assert(varint.to_number() == number);
        assert(varint.to_string() == string);
        assert(varint.byte_size() == string.size());
    }
}

static void test_varints_parsing() {
    VarInt varint;
    std::string_view output;

    for (auto [number, string] : known_varints_map) {
        assert(varint.parse(string, output));
        assert(output.empty());

        assert(varint.to_number() == number);
        assert(varint.to_string() == string);
        assert(varint.byte_size() == string.size());

        // Testing parsing leftover
        std::string str = string + "abc";
        assert(varint.parse(str, output));
        assert(output == "abc");

        assert(varint.to_number() == number);
        assert(varint.to_string() == string);
        assert(varint.byte_size() == string.size());
    }
}

static void test_varints_ordering() {
    std::vector<VarInt> sorted_vector = {0, 128, 200, 256, 831, 1000, 12345, 123456, 184467440737};
    std::vector<VarInt> vector = sorted_vector;
    std::random_device rd;
    std::mt19937_64 generator(rd());

    while (sorted_vector == vector) {
        std::cout << "Shuffling..." << std::endl;
        std::shuffle(vector.begin(), vector.end(), generator);
    }
    assert(sorted_vector != vector);
    std::sort(vector.begin(), vector.end());
    assert(sorted_vector == vector);
}

static void test_back_forth_varints() {
    VarInt v;
    std::string_view out;
    std::string in;

    for (std::uint64_t i = 0; i < 0xFFFF; i++) {
        v.reset(i);
        in = v.to_string();

        assert(v.parse(in, out));
        assert(out.empty());

        assert(v.to_number() == i);
        assert(v.to_string() == in);
    }

    // Skipping a few...

    for (std::uint64_t i = 0xFFFFFFFFFFFF0000; i < 0xFFFFFFFFFFFFFFFF; i++) {
        v.reset(i);
        in = v.to_string();

        assert(v.parse(in, out));
        assert(out.empty());

        assert(v.to_number() == i);
        assert(v.to_string() == in);
    }
}

int Utils_TestVarInt(int, char**)
{
    test_varints_7bits();
    test_varints_after_8bits();
    test_known_varints();

    test_varints_ordering();
    test_varints_parsing();

    test_back_forth_varints();
    return 0;
}
