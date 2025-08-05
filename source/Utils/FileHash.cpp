/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 22:13:15 2023 Francois Michaut
** Last update Wed Aug 20 16:40:49 2025 Francois Michaut
**
** FileHash.cpp : Function to hash file contents
*/

#include "FileShare/Utils/DebugPerf.hpp"
#include "FileShare/Utils/FileDescriptor.hpp"
#include "FileShare/Utils/FileHash.hpp"

#include <CppSockets/OSDetection.hpp>
#include <CppSockets/Tls/Utils.hpp>

#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include <cstdio>
#include <vector>

#ifdef OS_UNIX
  #include <fcntl.h>
#endif

constexpr auto READ_SIZE = 0x8000; // = 32_768 bytes

// TODO: custom exceptions with errno support when needed
namespace FileShare::Utils {
    auto file_hash(HashAlgorithm algo, const std::filesystem::path &path) -> std::string {
        DebugPerf debug("file_hash");

        CppSockets::EVP_MD_ptr digest {EVP_MD_fetch(nullptr, algo_to_string(algo), nullptr)};
        CppSockets::EVP_MD_CTX_ptr ctx {EVP_MD_CTX_new()};
        std::array<unsigned char, EVP_MAX_MD_SIZE> digest_buff = {0};
        unsigned int output_size;
        FileHandle file(path, "r");
        std::size_t ret = READ_SIZE;
        std::vector<char> buff(READ_SIZE);

#if defined(OS_UNIX) && !defined(OS_APPLE)
        posix_fadvise(file.fd(false), 0, 0, POSIX_FADV_SEQUENTIAL); // Ignoring return - this is optional
#endif
        if (!digest || !ctx)
            throw std::runtime_error("Failed to intialize the hash context");

        EVP_MD_CTX_set_flags(ctx.get(), EVP_MD_CTX_FLAG_FINALISE);
        if (EVP_DigestInit(ctx.get(), digest.get()) <= 0)
            throw std::runtime_error("Failed to init the digest context");

        while (ret == READ_SIZE) {
#if defined (OS_UNIX) && !defined (OS_APPLE)
            ret = fread_unlocked(buff.data(), 1, READ_SIZE, file);
#else
            ret = fread(buff.data(), 1, READ_SIZE, file);
#endif

            if (ret == 0)
                break;

            if (ret < 0)
                throw std::runtime_error("File read failed");
            if (EVP_DigestUpdate(ctx.get(), buff.data(), ret) <= 0)
                throw std::runtime_error("Failed to hash file data");
        }
        if (EVP_DigestFinal(ctx.get(), digest_buff.data(), &output_size) <= 0)
            throw std::runtime_error("Failed to compute the file hash");
        if (output_size != algo_hash_size(algo))
            throw std::runtime_error("Hash size is not what was expected");
        return {reinterpret_cast<char *>(digest_buff.data()), output_size};
    }

    auto algo_hash_size(HashAlgorithm algo) -> std::size_t{
        switch (algo) {
            case HashAlgorithm::MD5:
                return MD5_DIGEST_LENGTH;
            case HashAlgorithm::SHA256:
                return SHA256_DIGEST_LENGTH;
            case HashAlgorithm::SHA512:
                return SHA512_DIGEST_LENGTH;
            default:
                throw std::runtime_error("Unknown hash algorithm value");
        }
    }

    auto algo_to_string(HashAlgorithm algo) -> const char * {
        switch (algo) {
            case HashAlgorithm::MD5:
                return "md5";
            case HashAlgorithm::SHA256:
                return "sha256";
            case HashAlgorithm::SHA512:
                return "sha512";
            default:
                throw std::runtime_error("Unknown hash algorithm value");
        }
    }
}
