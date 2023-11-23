/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 11:08:04 2023 Francois Michaut
** Last update Wed Nov 22 20:25:28 2023 Francois Michaut
**
** TestProtocolVersion.cpp : ProtocolVersion tests
*/

#include "FileShare/Protocol/Version.hpp"

#include <algorithm>
#include <cassert>
#include <map>
#include <random>
#include <iostream>
#include <vector>

using namespace FileShare::Protocol;

static void test_spaceship_operator(std::vector<Version> &sorted_vector) {
    std::random_device rd;
    std::mt19937_64 generator(rd());

    std::vector<Version> vector = sorted_vector;

    while (sorted_vector == vector) {
        std::cout << "Shuffling..." << std::endl;
        std::shuffle(vector.begin(), vector.end(), generator);
    }
    assert(sorted_vector != vector);
    std::sort(vector.begin(), vector.end());
    assert(sorted_vector == vector);
}

static void test_version_decomposition(std::map<Version, std::array<std::uint8_t, 3>> &versions) {
    for (auto const& [key, val] : versions) {
        assert(key.major() == val[0]);
        assert(key.minor() == val[1]);
        assert(key.patch() == val[2]);
    }

    Version v3_0_0_errored = (Version::VersionEnum)(0x030001);
    std::array<std::uint8_t, 3> expected = {3, 0, 0};

    assert(v3_0_0_errored.major() == expected[0]);
    assert(v3_0_0_errored.minor() == expected[1]);
    assert(v3_0_0_errored.patch() != expected[2]);
}

int Protocol_TestVersion(int, char**)
{
    Version::VersionEnum v0_0_0 = (Version::VersionEnum)(0x000000);
    Version::VersionEnum v1_1_1 = (Version::VersionEnum)(0x010101);
    Version::VersionEnum v2_5_10 = (Version::VersionEnum)(0x02050A);
    Version::VersionEnum v2_5_11 = (Version::VersionEnum)(0x02050B);
    Version::VersionEnum v2_6_0 = (Version::VersionEnum)(0x020600);
    Version::VersionEnum v3_0_0 = (Version::VersionEnum)(0x030000);

    std::vector<Version> vector = {v0_0_0, v1_1_1, v2_5_10, v2_5_11, v2_6_0, v3_0_0};
    std::map<Version, std::array<std::uint8_t, 3>> versions = {
      {v0_0_0, {0, 0, 0}},
      {v1_1_1, {1, 1, 1}},
      {v2_5_10, {2, 5, 10}},
      {v2_5_11, {2, 5, 11}},
      {v2_6_0, {2, 6, 0}},
      {v3_0_0, {3, 0, 0}}
    };

    test_spaceship_operator(vector);
    test_version_decomposition(versions);
    return 0;
}
