/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 22:13:15 2023 Francois Michaut
** Last update Sun May 14 20:19:13 2023 Francois Michaut
**
** FileHash.cpp : Function to hash file contents
*/

#include "FileShare/Utils/FileDescriptor.hpp"
#include "FileShare/Utils/FileHash.hpp"
#include "FileShare/Utils/DebugPerf.hpp"

#include <CppSockets/OSDetection.hpp>
#include <CppSockets/TlsSocket.hpp>

#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#include <vector>

#include <stdio.h>

#ifdef OS_UNIX
  #include <fcntl.h>
#endif

#define READ_SIZE 0x8000 // = 32_768 bytes

// TODO: custom exceptions with errno support when needed
namespace FileShare::Utils {
    std::string file_hash(HashAlgorithm algo, const std::filesystem::path &path) {
        DebugPerf debug("file_hash");

        CppSockets::EVP_MD_ptr md;
        CppSockets::EVP_MD_CTX_ptr ctx;
        unsigned char digest_buff[EVP_MAX_MD_SIZE];
        unsigned int output_size;
        FileHandle file(path, "r");
        std::size_t ret = READ_SIZE;
        std::vector<char> buff(READ_SIZE);

#ifdef OS_UNIX
        posix_fadvise(file.fd(false), 0, 0, POSIX_FADV_SEQUENTIAL); // Ignoring return - this is optional
#endif
        md = {EVP_MD_fetch(nullptr, algo_to_string(algo), nullptr), EVP_MD_free};
        ctx = {EVP_MD_CTX_new(), EVP_MD_CTX_free};
        if (!md || !ctx)
            throw std::runtime_error("Failed to intialize the hash context");

        EVP_MD_CTX_set_flags(ctx.get(), EVP_MD_CTX_FLAG_FINALISE);
        if (EVP_DigestInit(ctx.get(), md.get()) <= 0)
            throw std::runtime_error("Failed to init the digest context");

        while (ret == READ_SIZE) {
            ret = fread_unlocked(buff.data(), 1, READ_SIZE, file);
            if (ret == 0)
                break;
            else if (ret < 0)
                throw std::runtime_error("File read failed");
            if (EVP_DigestUpdate(ctx.get(), buff.data(), ret) <= 0)
                throw std::runtime_error("Failed to hash file data");
        }
        if (EVP_DigestFinal(ctx.get(), digest_buff, &output_size) <= 0)
            throw std::runtime_error("Failed to compute the file hash");
        return std::string((char *)digest_buff, output_size);
    }

    std::size_t algo_hash_size(HashAlgorithm algo) {
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

    const char *algo_to_string(HashAlgorithm algo) {
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
