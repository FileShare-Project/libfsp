/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Mon Jul 28 19:24:26 2025 Francois Michaut
** Last update Sat Aug 23 11:06:22 2025 Francois Michaut
**
** PeerBase.cpp : Implementation of the shared Base for the Peer class
*/

#include "FileShare/Peer/PeerBase.hpp"

#include <CppSockets/Tls/Certificate.hpp>
#include <CppSockets/Tls/Utils.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <openssl/asn1.h>
#include <stdexcept>
#include <utility>

namespace FileShare {
    PeerBase::PeerBase(const CppSockets::IEndpoint &peer, CppSockets::TlsContext ctx) :
        m_socket(AF_INET, SOCK_STREAM, 0, std::move(ctx))
    {
        m_socket.connect(peer);
        read_peer_certificate();
    }

    PeerBase::PeerBase(CppSockets::TlsSocket &&peer) {
        if (!peer.connected()) {
            throw std::runtime_error("Socket is not connected");
        }
        m_socket = std::move(peer);
        read_peer_certificate();
    }

    void PeerBase::disconnect() {
        m_socket.close();
    }

    void PeerBase::read_peer_certificate() {
        const auto &raw_cert = m_socket.get_peer_cert();

        if (!raw_cert) {
            throw std::runtime_error("Peer didn't provide a Certificate");
        }
        CppSockets::Certificate cert {raw_cert.get(), false};
        // TODO: Validate certificate or throw + get uuid / name / public_key

        if (!cert.verify()) {
            throw std::runtime_error("Invalid Certificate");
        }
        const auto &raw_key = cert.get_pubkey();
        const auto &subject = cert.get_subject_name();
        const auto *device_uuid = subject.get_entry(NID_dnQualifier).get_data();
        const auto *device_name = subject.get_entry(NID_commonName).get_data();

        const auto *device_uuid_str = ASN1_STRING_get0_data(device_uuid);
        const auto *device_name_str = ASN1_STRING_get0_data(device_name);
        CppSockets::BIO_ptr public_key {BIO_new(BIO_s_mem())};
        char *key_bytes;
        std::size_t nb_bytes;

        // TODO: We dont want PEM. Use smth else
        // Use X509_pubkey_digest ?
        if (!PEM_write_bio_PUBKEY(public_key.get(), raw_key)) {
            throw std::runtime_error("Failed to fetch public key");
        }
        nb_bytes = BIO_get_mem_data(public_key.get(), &key_bytes); // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)

        m_public_key = {key_bytes, nb_bytes};
        m_device_uuid = {reinterpret_cast<const char *>(device_uuid_str), static_cast<std::size_t>(ASN1_STRING_length(device_uuid))};
        m_device_name = {reinterpret_cast<const char *>(device_name_str), static_cast<std::size_t>(ASN1_STRING_length(device_name))};
    }

    void PeerBase::poll_requests() {
        std::size_t total = 0;
        std::string_view view;
        Protocol::Request request;
        std::size_t ret = 0;

        if (!m_socket.connected()) // TODO: Check if there is still buffered bytes
            return;
        m_buffer += m_socket.read(); // TODO: add a timeout
        if (m_buffer.empty()) {
            return;
        }
        view = m_buffer;
        while (true) {
            ret = parse_bytes(view, request);
            if (ret == 0) {
                break;
            }
            // TODO: if this raises an error, we risk to re-process the same requests
            // (buffer not truncated)
            authorize_request(request);
            total += ret;
            view = view.substr(ret);
        }
        m_buffer = m_buffer.substr(total);
    }
}
