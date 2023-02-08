/*
** Project FileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Nov  6 21:06:10 2022 Francois Michaut
** Last update Tue Feb  7 23:45:26 2023 Francois Michaut
**
** Server.cpp : Server implementation
*/

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#include "FileShareProtocol/Server.hpp"

namespace FileShareProtocol {
    Server::Server(Config config) :
        m_socket(CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0)), m_config(std::move(config))
    {
        initialize_private_key();
        initialize_download_directory();
    }

    Client &Server::connect(CppSockets::TlsSocket &&peer) {
        return connect(std::move(peer), m_config);
    }

    Client &Server::connect(const CppSockets::IEndpoint &peer) {
        return connect(peer, m_config);
    }

    Client &Server::connect(CppSockets::TlsSocket &&peer, const Config &config) {
        return m_client_list.emplace_back(std::move(peer), config);
    }

    Client &Server::connect(const CppSockets::IEndpoint &peer, const Config &config) {
        return m_client_list.emplace_back(peer, config);
    }

    const Config &Server::get_config() const {
        return m_config;
    }

    void Server::set_config(const Config &config) {
        m_config = config;
    }

    const CppSockets::TlsSocket &Server::get_socket() const {
        return m_socket;
    }

    Config Server::default_config()
    {
        return {}; // TODO: explicitely set default params
    }

    void Server::initialize_download_directory() {
        std::filesystem::directory_entry downloads{m_config.get_downloads_folder()};

        if (!downloads.exists()) {
            std::filesystem::create_directory(downloads.path());
            // TODO: change permissions ? Defaults are 777.
        } else if (!downloads.is_directory()) {
            throw std::runtime_error("The download destination is not a directory");
        }
    }

    void Server::initialize_private_key() {
        auto key_path = std::filesystem::path(m_config.get_private_keys_dir()) / (m_config.get_private_key_name() + "_key.pem");
        auto cert_path = std::filesystem::path(m_config.get_private_keys_dir()) / (m_config.get_private_key_name() + "_cert.pem");
        bool key_exists = std::filesystem::exists(key_path);
        bool cert_exists = std::filesystem::exists(cert_path);

        // Setup dir & permissions
        std::filesystem::create_directories(m_config.get_private_keys_dir());
        std::filesystem::permissions(m_config.get_private_keys_dir(), Config::secure_folder_perms, std::filesystem::perm_options::replace);

        // Generate private key/certificate if not present
        if (!key_exists && !cert_exists) {
            CppSockets::EVP_PKEY_ptr pkey = {EVP_RSA_gen(4096), EVP_PKEY_free};
            CppSockets::X509_ptr x509 = {X509_new(), X509_free};
            int tls_ret = 1;

            if (pkey == nullptr ||
                (tls_ret = ASN1_INTEGER_set(X509_get_serialNumber(x509.get()), 1)) < 1 ||
                (X509_gmtime_adj(X509_getm_notBefore(x509.get()), 0)) == nullptr ||
                (X509_gmtime_adj(X509_getm_notAfter(x509.get()), 31536000L)) == nullptr || // TODO: implement key rotation / change this value of 1year?
                (tls_ret = X509_set_pubkey(x509.get(), pkey.get())) < 1
            ) {
                if (tls_ret < 1) {
                    throw std::runtime_error("Failed to generate key/certificate pair: " + std::string(ERR_error_string(ERR_get_error(), nullptr)));
                } else if (pkey == nullptr) {
                    throw std::runtime_error("Failed to generate key/certificate pair: 'EVP_RSA_gen' returned NULL");
                } else {
                    throw std::runtime_error("Failed to generate key/certificate pair: 'X509_gmtime_adj' returned NULL");
                }
            }
            X509_NAME *name = X509_get_subject_name(x509.get());

            if ((X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char *)"FileShare", -1, -1, 0) < 1) || // NOLINT(hicpp-signed-bitwise)
                (X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"TODO_PUT_UUID_HERE", -1, -1, 0) < 1) || // NOLINT(hicpp-signed-bitwise)
                (X509_set_issuer_name(x509.get(), name) < 1) ||
                (X509_sign(x509.get(), pkey.get(), EVP_sha256()) <= 0) // TODO: make it configurable ? (TODO: find what I wanted to make configurable about this)
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
            if (PEM_write_PrivateKey(pkey_file, pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) < 1 ||
                PEM_write_X509(cert_file, x509.get()) < 1
            ) {
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
            std::filesystem::permissions(key_path, Config::secure_file_perms, ec);
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
        if (std::filesystem::status(key_path).permissions() != Config::secure_file_perms) {
            // TODO maybe add a param to force set ?
            throw std::runtime_error("Insecure permissions for the private key !");
        }
    }
}
