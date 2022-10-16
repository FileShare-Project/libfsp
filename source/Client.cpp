/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Mon Oct 24 21:19:51 2022 Francois Michaut
**
** Client.cpp : Implementation of the FileShareProtocol Client
*/

#include "FileShareProtocol/Client.hpp"

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <stdexcept>

// TODO: Use custom classes for exceptions
namespace FileShareProtocol {
    Client::Client(const CppSockets::IEndpoint &peer, ClientConfig config, bool create_key_if_missing) :
        socket(CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0)), config(std::move(config))
    {
        if (create_key_if_missing)
            initialize_private_key();
        initialize_download_directory();
        reconnect(peer);
    }

    Client::Client(CppSockets::TlsSocket &&peer, ClientConfig config, bool create_key_if_missing) :
        socket(CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0)), config(std::move(config))
    {
        if (create_key_if_missing)
            initialize_private_key();
        initialize_download_directory();
        reconnect(std::move(peer));
    }

    const ClientConfig &Client::get_config() const {
        return config;
    }

    void Client::set_config(const ClientConfig &config) {
        this->config = config;
    }

    const CppSockets::TlsSocket &Client::get_socket() const {
        return socket;
    }

    void Client::reconnect(const CppSockets::IEndpoint &peer) {
        socket = CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0); // TODO check if needed
        socket.connect(peer);
        // TODO authentification + protocol handshake
    }

    void Client::reconnect(CppSockets::TlsSocket &&peer) {
        if (!peer.connected()) {
            throw std::runtime_error("Socket is not connected");
        }
        socket = std::move(peer);
        // TODO authentification + protocol handshake
    }

    ClientConfig Client::default_config()
    {
        return {}; // TODO: explicitely set default params
    }

    void Client::initialize_download_directory() {
        std::filesystem::directory_entry downloads{config.get_downloads_folder()};

        if (!downloads.exists()) {
            std::filesystem::create_directory(downloads.path());
            // TODO: change permissions ? Defaults are 777.
        } else if (!downloads.is_directory()) {
            throw std::runtime_error("The download destination is not a directory");
        }
    }

    void Client::initialize_private_key() {
        auto key_path = std::filesystem::path(config.get_private_keys_dir()) / (config.get_private_key_name() + "_key.pem");
        auto cert_path = std::filesystem::path(config.get_private_keys_dir()) / (config.get_private_key_name() + "_cert.pem");
        bool key_exists = std::filesystem::exists(key_path);
        bool cert_exists = std::filesystem::exists(cert_path);

        // Setup dir & permissions
        std::filesystem::create_directories(config.get_private_keys_dir());
        std::filesystem::permissions(config.get_private_keys_dir(), ClientConfig::secure_folder_perms, std::filesystem::perm_options::replace);

        // Generate private key/certificate if not present
        if (!key_exists && !cert_exists) {
            CppSockets::EVP_PKEY_ptr pkey = {EVP_PKEY_new(), EVP_PKEY_free};
            CppSockets::RSA_ptr rsa = {RSA_new(), RSA_free};
            CppSockets::X509_ptr x509 = {X509_new(), X509_free};
            CppSockets::BIGNUM_ptr exponent = {BN_new(), BN_free};
            int tls_ret = 0;

            if (!((tls_ret = BN_set_word(exponent.get(), RSA_F4)) == 1 &&
                  (tls_ret = RSA_generate_key_ex(rsa.get(), 4096, exponent.get(), nullptr)) == 1 &&
                  (tls_ret = EVP_PKEY_set1_RSA(pkey.get(), rsa.get())) == 1 &&
                  (tls_ret = ASN1_INTEGER_set(X509_get_serialNumber(x509.get()), 1)) == 1 &&
                  (X509_gmtime_adj(X509_getm_notBefore(x509.get()), 0)) != nullptr &&
                  (X509_gmtime_adj(X509_getm_notAfter(x509.get()), 31536000L)) != nullptr && // TODO: implement key rotation / change this value of 1year?
                  (tls_ret = X509_set_pubkey(x509.get(), pkey.get())) == 1)
            ) {
                if (tls_ret == 0) {
                    throw std::runtime_error("Failed to generate key/certificate pair: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
                } else {
                    throw std::runtime_error("Failed to generate key/certificate pair: 'X509_gmtime_adj' returned NULL");
                }
            }
            X509_NAME *name = X509_get_subject_name(x509.get());

            // X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char *)"CA", -1, -1, 0);
            if (!((X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"FileShare", -1, -1, 0) == 1) && // NOLINT(hicpp-signed-bitwise)
                  (X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"TODO_PUT_UUID_HERE", -1, -1, 0) == 1) && // NOLINT(hicpp-signed-bitwise)
                  (X509_set_issuer_name(x509.get(), name) == 1) &&
                  (X509_sign(x509.get(), pkey.get(), EVP_sha256()) != 0)) // TODO: make it configurable ?
            ) {
                throw std::runtime_error("Failed to sign certificate: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
            }
            std::FILE *pkey_file = std::fopen(key_path.string().c_str(), "w");
            std::FILE *cert_file = std::fopen(cert_path.string().c_str(), "w");

            if (pkey_file == nullptr || cert_file == nullptr) {
                pkey_file != nullptr ? std::fclose(pkey_file) : 0;
                cert_file != nullptr ? std::fclose(cert_file) : 0;
                throw std::runtime_error("Failed to open '" + (pkey_file == nullptr ? key_path.string() : cert_path.string()) + "' for writing");
            }
            std::error_code ec;

            // TODO: encrypt private key
            if (!(PEM_write_PrivateKey(pkey_file, pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) == 1 &&
                  PEM_write_X509(cert_file, x509.get()) == 1)) {
                std::fclose(pkey_file);
                std::fclose(cert_file);
                std::filesystem::remove(key_path, ec);
                std::filesystem::remove(cert_path);
                if (ec)
                    throw std::runtime_error(ec.message());
                throw std::runtime_error("Failed to write key/certificate to file");
            }
            if (std::fclose(pkey_file) + std::fclose(cert_file) != 0) {
                std::filesystem::remove(key_path, ec);
                std::filesystem::remove(cert_path);
                if (ec)
                    throw std::runtime_error(ec.message());
                throw std::runtime_error("Failed to save key/certificate to disk");
            }
            std::filesystem::permissions(key_path, ClientConfig::secure_file_perms, ec);
            if (ec) {
                std::error_code ec2;

                std::filesystem::remove(key_path, ec2);
                std::filesystem::remove(cert_path);
                if (ec2)
                    throw std::runtime_error(ec2.message());
                throw std::runtime_error("Failed to set secure permissions on the private key: " + ec.message());
            }
            return;
        } else if (key_exists ^ cert_exists) {
            // TODO maybe add a param to force-generate ?
            throw std::runtime_error("Found a private key or a certificate but not both"); // TODO: maybe missing certificate is ok if we implement key rotation ?
        }
        if (std::filesystem::status(key_path).permissions() != ClientConfig::secure_file_perms) {
            // TODO maybe add a param to force set ?
            throw std::runtime_error("Insecure permissions for the private key !");
        }
    }
}
