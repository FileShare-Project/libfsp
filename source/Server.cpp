/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Nov  6 21:06:10 2022 Francois Michaut
** Last update Fri Aug 22 23:57:25 2025 Francois Michaut
**
** Server.cpp : Server implementation
*/

#include "FileShare/Server.hpp"
#include "CppSockets/Tls/Socket.hpp"
#include "FileShare/Utils/Poll.hpp"
#include "FileShare/Utils/Vector.hpp"

#include <CppSockets/Tls/Certificate.hpp>
#include <CppSockets/Tls/Utils.hpp>

#include <cstddef>
#include <memory>
#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#include <stdexcept>
#include <string>
#include <utility>

constexpr const auto ORG_NAME = u8"FileShare";
constexpr const auto ORG_NAME_SIZE = std::char_traits<char8_t>::length(ORG_NAME);
constexpr const auto ORG_UNIT_NAME = u8"FileShare Self-Signed Device Certificate";
constexpr const auto ORG_UNIT_NAME_SIZE = std::char_traits<char8_t>::length(ORG_UNIT_NAME);

namespace {
    // TODO: Once we have a central server, make that server sign certificates so we dont have
    // to rely on self-signed ones. Self-Signed will only be used on Offline networks.
    auto verify_callback(int preverify_ok, X509_STORE_CTX *ctx) -> int {
        int err = X509_STORE_CTX_get_error(ctx); // man X509_STORE_CTX_get_error
        static std::basic_string_view<char8_t> org_name_cmp {ORG_NAME, ORG_NAME_SIZE};
        static std::basic_string_view<char8_t> org_unit_name_cmp {ORG_UNIT_NAME, ORG_UNIT_NAME_SIZE};

        // If we want to get the Server instance of that SSL connection :
        // SSL *ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
        // FileShare::Server *server = SSL_get_ex_data(ssl, mydata_index); -> use SSL_get_ex_new_index to get index

        if (err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT) {
            // This only returns a certificate if there was an error
            X509 *err_cert = X509_STORE_CTX_get_current_cert(ctx);

            // TODO: Need to check before / after date ?
            // TODO: Find a way to check revoked certificates -> https://en.wikipedia.org/wiki/Certificate_revocation_list

            X509_NAME *subject = X509_get_subject_name(err_cert);
            int org_name_index = X509_NAME_get_index_by_NID(subject, NID_organizationName, -1);
            int org_unit_name_index = X509_NAME_get_index_by_NID(subject, NID_organizationalUnitName, -1);
            // int user_id_index = X509_NAME_get_index_by_NID(subject, NID_userId, -1);
            int common_name_index = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
            int dn_qualifier_index = X509_NAME_get_index_by_NID(subject, NID_dnQualifier, -1);

            X509_NAME_ENTRY *org_name = X509_NAME_get_entry(subject, org_name_index);
            X509_NAME_ENTRY *org_unit_name = X509_NAME_get_entry(subject, org_unit_name_index);
            // X509_NAME_ENTRY *user_id = X509_NAME_get_entry(subject, user_id_index);
            X509_NAME_ENTRY *common_name = X509_NAME_get_entry(subject, common_name_index);
            X509_NAME_ENTRY *dn_qualifier = X509_NAME_get_entry(subject, dn_qualifier_index);

            bool cert_valid = org_name && org_unit_name && /* user_id && */common_name && dn_qualifier;

            if (cert_valid) {
                ASN1_STRING *org_name_str = X509_NAME_ENTRY_get_data(org_name);
                ASN1_STRING *org_unit_name_str = X509_NAME_ENTRY_get_data(org_unit_name);
                // ASN1_STRING *user_id_str = X509_NAME_ENTRY_get_data(user_id);
                // ASN1_STRING *dn_qualifier_str = X509_NAME_ENTRY_get_data(dn_qualifier);

                const auto *org_name_data = reinterpret_cast<const char8_t *>(
                    ASN1_STRING_get0_data(org_name_str)
                );
                const auto *org_unit_name_data = reinterpret_cast<const char8_t *>(
                    ASN1_STRING_get0_data(org_unit_name_str)
                );

                std::basic_string_view<char8_t> org_name_view {
                    org_name_data, static_cast<std::size_t>(ASN1_STRING_length(org_name_str))
                };
                std::basic_string_view<char8_t> org_unit_name_view {
                    org_unit_name_data, static_cast<std::size_t>(ASN1_STRING_length(org_unit_name_str))
                };

                // TODO: Do this properly by converting encodings, so we can accept certs with different encodings
                // File with available X509 fields NIDs : /usr/include/openssl/obj_mac.h
                // File with available string types + max_lenght of X509 fields : /usr/include/openssl/asn1.h
                if (org_name_view != org_name_cmp || org_unit_name_view != org_unit_name_cmp) {
                    cert_valid = false;
                }
                // TODO: Verify user_id_str && dn_qualifier_str are in UUID format (user_id_str is optional)
            }

            if (!cert_valid) {
                X509_STORE_CTX_set_error(ctx, X509_V_ERR_APPLICATION_VERIFICATION);
                return 0;
            }
            X509_STORE_CTX_set_error(ctx, X509_V_OK);
            return 1;
        }
        // If there is an error other than SELF_SIGNED_CERT, we let it bubble up
        return preverify_ok;
    }
}

namespace FileShare {
    // NOLINTNEXTLINE(hicpp-signed-bitwise)
    static constexpr int VERIFY_MODE = SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT | SSL_VERIFY_CLIENT_ONCE;

    Server::Server(ServerConfig config, Config peer_config) :
        Server(Server::default_endpoint(), std::move(config), std::move(peer_config))
    {}

    Server::Server(std::shared_ptr<CppSockets::IEndpoint> server_endpoint, ServerConfig config, Config peer_config) :
        m_server_endpoint(std::move(server_endpoint)), m_ctx(SSL_CTX_new(TLS_method())),
        m_config(std::move(config)), m_peer_config(std::move(peer_config))
    {
        // Request for client certificate + verify it
        m_ctx.set_verify(VERIFY_MODE, verify_callback);
        restart();
    }

    // "~Server() = default" needs to be defined in the source file,
    // cause the header has an incomplete definition of struct pollfd
    Server::~Server() = default;

    void Server::restart() {
        auto base_path = std::filesystem::path(m_config.get_private_keys_dir());
        auto key_path = base_path / (m_config.get_private_key_name() + "_key.pem");
        auto cert_path = base_path / (m_config.get_private_key_name() + "_cert.pem");

        initialize_private_key();
        this->m_ctx.set_certificate(cert_path.generic_string(), key_path.generic_string());

        if (m_fds.empty()) {
            m_fds.emplace_back(pollfd{.fd = -1, .events = POLLIN, .revents = 0}); // Create server socket slot
        }

        if (!this->disabled()) {
            // Required in case the CTX params changes - SSL Sockets dont pick up on changes otherwise
            m_socket = CppSockets::TlsSocket(AF_INET, SOCK_STREAM, 0, m_ctx);
            m_socket.set_reuseaddr(true);
            // TODO: Set non-blocking ?
            m_socket.bind(*this->m_server_endpoint);
            m_socket.listen(10); // TODO: configurable backlog
            m_fds[0].fd = m_socket.get_fd();
        } else {
            m_fds[0].fd = -1;
            m_socket.close();
        }
    }

    void Server::set_disabled(bool disabled) {
        m_config.set_server_disabled(disabled);
        restart();
    }

    auto Server::connect(CppSockets::TlsSocket peer, const Config &config) -> std::shared_ptr<Peer> & {
        PreAuthPeer pre_auth(std::move(peer), PreAuthPeer::CLIENT);

        // TODO: Bad. Change it
        pre_auth.do_client_hello();
        while (!pre_auth.has_protocol() && pre_auth.get_socket().connected()) {
            pre_auth.poll_requests();
        }

        std::shared_ptr<Peer> client = std::make_shared<Peer>(std::move(pre_auth), config);

        m_fds.emplace_back(pollfd({.fd = client->get_socket().get_fd(), .events = POLLIN, .revents = 0}));
        return insert_peer(std::move(client));
    }

    auto Server::connect(const CppSockets::IEndpoint &peer, const Config &config) -> std::shared_ptr<Peer> & {
        CppSockets::TlsSocket socket(AF_INET, SOCK_STREAM, 0, m_ctx);

        socket.connect(peer);
        return connect(std::move(socket), config);
    }

    void Server::process_events(const PeerAcceptCallback &accept_cb, const PeerRequestCallback &request_cb) {
        poll_events();

        for (auto &iter : m_events) {
            auto type = iter.type();

            if (type == Event::REQUEST) {
                auto peer = std::dynamic_pointer_cast<Peer>(iter.peer());

                request_cb(*this, peer, iter.request().value());
            } else if (type == Event::CONNECT) {
                auto peer = std::dynamic_pointer_cast<PreAuthPeer>(std::move(iter.peer()));

                if (accept_cb(*this, peer)) {
                    accept_peer(std::move(peer));
                }
            }
        }
        m_events.clear();
    }

    auto Server::pull_event(Event &result) -> bool {
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

    auto Server::default_config() -> ServerConfig {
        return {}; // TODO: explicitely set default params
    }

    auto Server::default_peer_config() -> Config {
        return {}; // TODO: explicitely set default params
    }

    auto Server::default_endpoint() -> std::shared_ptr<CppSockets::IEndpoint> {
        // TODO: choose a better port than 12345
        return std::make_shared<CppSockets::EndpointV4>(CppSockets::IPv4("127.0.0.1"), 12345);
    }

    void Server::accept_peer(PreAuthPeer_ptr peer, bool temporary_trust) {
        if (!temporary_trust) {
            // Add to known hosts
            m_known_peers.insert(peer->get_device_uuid(), peer->get_public_key());
        }
        m_pending_authorization_peers.erase(peer->get_socket().get_fd());
        insert_peer(std::move(peer));
    }

    auto Server::insert_peer(PreAuthPeer_ptr &&peer) -> Peer_ptr & {
        auto new_peer = std::make_shared<Peer>(std::move(*peer), m_peer_config);

        return insert_peer(new_peer);
    }

    auto Server::insert_peer(Peer_ptr peer) -> Peer_ptr & {
        RawSocketType client_fd = peer->get_socket().get_fd();
        const auto &result = m_peers.emplace(client_fd, std::move(peer));
        const auto &iter = result.first;

        if (!result.second) {
            throw std::runtime_error("Peer already connected");
        }
        return iter->second;
    }

    auto Server::delete_peer(FdVector::iterator iter) -> FdVector::iterator {
        m_peers.erase(iter->fd);
        return delete_move(m_fds, iter);
    }

    auto Server::delete_pre_auth_peer(FdVector::iterator iter, PreAuthPeerMap &map) -> FdVector::iterator {
        map.erase(iter->fd);
        return delete_move(m_fds, iter);
    }

    void Server::poll_events() {
        // TODO: configurable wait (currently 1s)
        struct timespec timeout = {.tv_sec = 1, .tv_nsec = 0};
        int nb_ready = Utils::poll(m_fds, &timeout);

        // if (nb_ready < 0) // TODO: handle signals
        //     throw std::runtime_error("Failed to poll");
        for (auto iter = m_fds.begin(); nb_ready > 0 && iter != m_fds.end(); ) {
            if (iter->revents & (POLLIN | POLLHUP)) { // NOLINT(hicpp-signed-bitwise)
                nb_ready--;

                // TODO: Add try-catch in case peer fails smth
                if (iter->fd == m_socket.get_fd()) {
                    std::unique_ptr<CppSockets::TlsSocket> tls_socket = m_socket.accept();
                    auto fd = tls_socket->get_fd();
                    auto peer = std::make_shared<PreAuthPeer>(
                        std::move(*tls_socket.release()), PreAuthPeer::SERVER
                    );

                    m_fds.emplace_back(
                        pollfd({.fd = fd, .events = POLLIN, .revents = 0})
                    );
                    m_handshake_peers.emplace(fd, std::move(peer));
                    iter++;
                } else {
                    iter = handle_peer_events(iter);
                }
            } else {
                iter++;
            }
        }
    }

    auto Server::handle_peer_events(FdVector::iterator iter) -> FdVector::iterator {
        auto peer_iter = m_peers.find(iter->fd);

        if (peer_iter != m_peers.end()) {
            std::shared_ptr<Peer> peer = peer_iter->second;
            std::vector<Protocol::Request> requests = peer->pull_requests();

            for (auto &iter : requests) {
                m_events.emplace_back(Event::REQUEST, peer, iter);
            }
            if (!peer->get_socket().connected()) {
                return delete_peer(iter);
            }
            return ++iter;
        }

        auto pending_peer_iter = m_pending_authorization_peers.find(iter->fd);

        if (pending_peer_iter != m_pending_authorization_peers.end()) {
            std::shared_ptr<PreAuthPeer> peer = pending_peer_iter->second;

            peer->poll_requests(); // Reject all Requests with Unauthorized status
            if (!peer->get_socket().connected()) {
                return delete_pre_auth_peer(iter, m_pending_authorization_peers);
            }
            return ++iter;
        }

        auto handshake_peer_iter = m_handshake_peers.find(iter->fd);

        if (handshake_peer_iter != m_handshake_peers.end()) {
            std::shared_ptr<PreAuthPeer> &peer = handshake_peer_iter->second;

            peer->poll_requests();
            if (!peer->get_socket().connected()) {
                return delete_pre_auth_peer(iter, m_handshake_peers);
            }
            if (peer->has_protocol()) {
                if (m_known_peers.contains(*peer)) {
                    // Already trusted peer
                    insert_peer(std::move(peer));
                } else {
                    // Not yet trusted peer, going through authorization step
                    auto inserted = m_pending_authorization_peers.emplace(iter->fd, std::move(peer));
                    std::shared_ptr<PreAuthPeer> new_peer = inserted.first->second;

                    m_events.emplace_back(Event::CONNECT, new_peer);
                }
                m_handshake_peers.erase(handshake_peer_iter);
            }
            return ++iter;
        }

        // Unknown FD -> Delete
        return delete_move(m_fds, iter);
    }

    void Server::initialize_private_key() {
        m_config.validate_config();

        // TODO: Knows Hosts File

        auto key_path = std::filesystem::path(m_config.get_private_keys_dir()) / (m_config.get_private_key_name() + "_key.pem");
        auto cert_path = std::filesystem::path(m_config.get_private_keys_dir()) / (m_config.get_private_key_name() + "_cert.pem");
        bool key_exists = std::filesystem::exists(key_path);
        bool cert_exists = std::filesystem::exists(cert_path);

        if (key_exists ^ cert_exists) {
            // TODO maybe add a param to force-generate ?
            throw std::runtime_error("Found a private key or a certificate but not both"); // TODO: maybe missing certificate is ok if we implement key rotation ?
        }

        // Setup dir & permissions
        std::filesystem::create_directories(m_config.get_private_keys_dir());
        std::filesystem::permissions(m_config.get_private_keys_dir(), ServerConfig::SECURE_FOLDER_PERMS, std::filesystem::perm_options::replace);

        // Generate private key/certificate if not present
        if (!cert_exists) {
            CppSockets::EVP_PKEY_ptr pkey {EVP_RSA_gen(4096)}; // NOLINT(cppcoreguidelines-pro-type-vararg, hicpp-vararg)
            CppSockets::x509Certificate cert;
            CppSockets::x509Name subject;
            // TODO: Replace this with "FSP_" + UUID (16 bytes) -> 20 bytes
            CppSockets::BIGNUM_ptr bignum {BN_bin2bn(reinterpret_cast<const unsigned char *>("FSP_0000000000000000"), 20, nullptr)};

            cert.set_public_key(pkey);

            // TODO: Actually put 20 random bytes here, this is used with issuer_name to revoke certs
            // so it should be uniq
            cert.set_serial_number(bignum.get());

            cert.set_not_before(0, 0);
            cert.set_not_after(365, 0); // TODO: implement key rotation / change this value of 1year?
            // For key rotation, can we cross-sign the new certificate with the old one ?
            // Or is that unsecure ?
            // https://ravendb.net/articles/how-cross-signing-works-with-x509-certificates

            // NOTE: All this data is NOT trusted, and should NOT be relied upon blindfully.
            // A malicious peer could craft a certificate containing any data to try to impersonate
            // another one. Only the certificate pkey can truly prove identity.

            // NOLINTBEGIN(hicpp-signed-bitwise)
            subject.add_entry(NID_organizationName, MBSTRING_ASC, ORG_NAME);
            subject.add_entry(NID_organizationalUnitName, MBSTRING_ASC, ORG_UNIT_NAME);
            subject.add_entry(NID_userId, MBSTRING_ASC, u8"TODO-PUT-USER-ID-HERE"); // TODO: When we have user accounts
            // TODO: Since DEVICE_NAME could change, it probably shouldn't be part of the subject_name
            // That would require the Server to re-generate certificate on name change, and if we
            // store certificates to remember them, it will invalidate the certificate because of a
            // name change...
            std::u8string uuid {reinterpret_cast<const char8_t *>(m_config.get_uuid().c_str()), m_config.get_uuid().size()};
            std::u8string device_name {reinterpret_cast<const char8_t *>(m_config.get_device_name().c_str()), m_config.get_device_name().size()};

            subject.add_entry(NID_commonName, MBSTRING_ASC, uuid);
            subject.add_entry(NID_dnQualifier, MBSTRING_ASC, device_name);
            // NOLINTEND(hicpp-signed-bitwise)

            cert.set_self_signed_name(subject);
            cert.sign(pkey);

            std::error_code ec;
            { // scope for BIO uniq ptr so we can control when they are freed/closed
                CppSockets::BIO_ptr pkey_file {BIO_new_file(key_path.string().c_str(), "w")};
                CppSockets::BIO_ptr cert_file {BIO_new_file(cert_path.string().c_str(), "w")};

                if (!pkey_file || !cert_file) {
                    throw std::runtime_error("Failed to open '" + (pkey_file == nullptr ? key_path.string() : cert_path.string()) + "' for writing");
                }

                // TODO: encrypt private key
                if (PEM_write_bio_PrivateKey(pkey_file.get(), pkey.get(), nullptr, nullptr, 0, nullptr, nullptr) < 1 ||
                    PEM_write_bio_X509(cert_file.get(), cert.get()) < 1
                ) {
                    std::filesystem::remove(key_path, ec);
                    std::filesystem::remove(cert_path);
                    if (ec)
                        throw std::runtime_error(ec.message());
                    throw std::runtime_error("Failed to write key/certificate to file");
                }
            } // Close both files
              // TODO: raise error if close fails

            std::filesystem::permissions(key_path, ServerConfig::SECURE_FILE_PERMS, ec);
            if (ec) {
                std::error_code ec2;

                std::filesystem::remove(key_path, ec2);
                std::filesystem::remove(cert_path);
                if (ec2)
                    throw std::runtime_error(ec2.message());
                throw std::runtime_error("Failed to set secure permissions on the private key: " + ec.message());
            }
            return;
        }

#ifndef OS_WINDOWS
        if (std::filesystem::status(key_path).permissions() != ServerConfig::SECURE_FILE_PERMS) {
            // TODO maybe add a param to force set ?
            throw std::runtime_error("Insecure permissions for the private key !");
        }
#endif
    }

    Server::Event::Event(Type type, PeerBase_ptr peer, std::optional<Protocol::Request> request) :
        m_type(type), m_peer(std::move(peer)), m_request(std::move(request))
    {}
}
