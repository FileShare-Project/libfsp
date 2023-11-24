/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Nov  6 21:06:10 2022 Francois Michaut
** Last update Tue Nov 28 13:46:19 2023 Francois Michaut
**
** Server.cpp : Server implementation
*/

#include "FileShare/Server.hpp"

#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>

#ifdef OS_UNIX
    #include <poll.h>
#endif

namespace FileShare {
    Server::Server(Config config) : Server(Server::default_endpoint(), std::move(config))
    {}

    Server::Server(std::shared_ptr<CppSockets::IEndpoint> server_endpoint, Config config) :
        m_server_endpoint(server_endpoint),
        m_config(std::move(config))
    {
        restart();
    }

    void Server::restart() {
        initialize_private_key();
        initialize_download_directory();
        if (!this->disabled()) {
            // TODO: avoid this duplication
            auto key_path = std::filesystem::path(m_config.get_private_keys_dir()) / (m_config.get_private_key_name() + "_key.pem");
            auto cert_path = std::filesystem::path(m_config.get_private_keys_dir()) / (m_config.get_private_key_name() + "_cert.pem");

            this->m_socket = CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0);
            this->m_socket.set_reuseaddr(true);
            this->m_socket.set_certificate(cert_path, key_path);
            this->m_socket.bind(*this->m_server_endpoint);
            this->m_socket.listen(10); // TODO: configurable backlog
        }
    }

    std::shared_ptr<Client> &Server::connect(CppSockets::TlsSocket peer) {
        return connect(std::move(peer), this->m_config);
    }

    std::shared_ptr<Client> &Server::connect(const CppSockets::IEndpoint &peer) {
        return connect(peer, this->m_config);
    }

    std::shared_ptr<Client> &Server::connect(CppSockets::TlsSocket peer, const Config &config) {
        // TODO: handle auth + version negotiation
        std::shared_ptr<Client> client = std::make_shared<Client>(std::move(peer), "TODO", "TODO", Protocol::Protocol(Protocol::Version::v0_0_0), config);

        return insert_client(std::move(client));
    }

    std::shared_ptr<Client> &Server::connect(const CppSockets::IEndpoint &peer, const Config &config) {
        // TODO: handle auth + version negotiation
        std::shared_ptr<Client> client = std::make_shared<Client>(peer, "TODO", "TODO", Protocol::Protocol(Protocol::Version::v0_0_0), config);

        return insert_client(std::move(client));
    }

    const Config &Server::get_config() const { return m_config; }
    void Server::set_config(const Config &config) { m_config = config; }
    const CppSockets::TlsSocket &Server::get_socket() const { return m_socket; }
    const CppSockets::IEndpoint &Server::get_server_endpoint() const { return *m_server_endpoint; }
    bool Server::disabled() const { return m_config.is_server_disabled(); }
    std::map<RawSocketType, std::shared_ptr<Client>> &Server::get_clients() { return m_clients; }

    void Server::process_events(ClientAcceptCallback accept_cb, ClientRequestCallback request_cb) {
        poll_events();

        for (auto iter = m_events.begin(); iter != m_events.end(); iter++) {
            auto request = iter->request();

            if (request.has_value()) {
                request_cb(*this, iter->client(), request.value());
            } else {
                if (accept_cb(*this, iter->client())) {
                    insert_client(iter->client());
                }
            }
        }
        m_events.clear();
    }

    bool Server::pull_event(Event &result) {
        if (m_events.empty())
            poll_events();
        if (m_events.empty()) {
            result = {};
            return false;
        }
        result = std::move(m_events.back());
        m_events.pop_back();
        return true;
    }

    Config Server::default_config()
    {
        return {}; // TODO: explicitely set default params
    }

    std::shared_ptr<CppSockets::IEndpoint> Server::default_endpoint()
    {
        // TODO: choose a better port than 12345
        return std::make_shared<CppSockets::EndpointV4>(CppSockets::IPv4("127.0.0.1"), 12345);
    }

    void Server::accept_client(std::shared_ptr<Client> peer) {
        insert_client(std::move(peer));
    }

    // TODO: do not accept double connection from clients
    std::shared_ptr<Client> &Server::insert_client(std::shared_ptr<Client> client) {
        const auto &result = m_clients.emplace(client->get_socket().get_fd(), std::move(client));

        return result.first->second; // TODO: check this does return a reference
    }

    void Server::poll_events() {
#ifdef OS_UNIX
        // TODO: avoid re-allocating the vector every time
        std::vector<struct pollfd> fds;
        nfds_t nfds = m_clients.size() + 1;
        struct timespec timeout = {1, 0}; // TODO: configurable wait (currently 1s)
        int nb_ready = 0;

        fds.reserve(nfds);
        fds.emplace_back(pollfd({m_socket.get_fd(), POLLIN, 0}));
        for (auto &iter : m_clients) {
            fds.emplace_back(pollfd({iter.first, POLLIN, 0}));
        }
#ifdef OS_APPLE
        nb_ready = poll(fds.data(), nfds, timeout.tv_sec * 1000);
#else
        nb_ready = ppoll(fds.data(), nfds, &timeout, nullptr);
#endif
        // if (nb_ready < 0) // TODO: handle signals
        //     throw std::runtime_error("Failed to poll");
        for (auto iter = fds.begin(); nb_ready > 0 && iter != fds.end(); iter++) {
            if (iter->revents & (POLLIN | POLLHUP)) {
                nb_ready++;
                if (iter->fd == m_socket.get_fd()) {
                    auto tls_socket =  m_socket.accept();
                    std::shared_ptr<Client> client = std::make_shared<Client>(std::move(*tls_socket), "TODO", "TODO", Protocol::Version::v0_0_0);
                    std::optional<Protocol::Request> req = {};

                    m_events.emplace_back(std::move(client), req);
                } else {
                    auto &client = m_clients.at(iter->fd);

                    if (!handle_client_events(client)) {
                        m_clients.erase(iter->fd);
                    }
                }
            }
        }
#else
        #error "Not implemented"
#endif
    }

    bool Server::handle_client_events(std::shared_ptr<Client> &client) {
        std::vector<Protocol::Request> requests = client->pull_requests();

        for (auto iter = requests.begin(); iter != requests.end(); iter++) {
            m_events.emplace_back(client, *iter);
        }
        if (!client->get_socket().connected()) {
            return false;
        }
        return true;
    }

    void Server::initialize_download_directory() {
        std::filesystem::directory_entry downloads{this->m_config.get_downloads_folder()};

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

    Server::Event::Event(std::shared_ptr<Client> client, std::optional<Protocol::Request> request) :
        m_client(std::move(client)), m_request(std::move(request))
    {}

    std::shared_ptr<Client> &Server::Event::client() {
        return m_client;
    }

    std::optional<Protocol::Request> &Server::Event::request() {
        return m_request;
    }
}
