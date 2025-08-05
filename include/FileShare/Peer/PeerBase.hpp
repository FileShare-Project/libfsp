/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Mon Jul 28 19:12:40 2025 Francois Michaut
** Last update Fri Aug 22 13:08:11 2025 Francois Michaut
**
** PeerBase.hpp : Base of the Peer class
*/

#pragma once

#include "FileShare/Protocol/Definitions.hpp"

#include <CppSockets/IPv4.hpp>
#include <CppSockets/Tls/Context.hpp>
#include <CppSockets/Tls/Socket.hpp>
#include <CppSockets/Version.hpp>

#include <memory>
#include <utility>

namespace FileShare {
    class PeerBase {
        public:
            virtual ~PeerBase() = default;

            PeerBase(const PeerBase &) = delete;
            PeerBase(PeerBase &&) = default;
            auto operator=(const PeerBase &) -> PeerBase & = delete;
            auto operator=(PeerBase &&) -> PeerBase & = default;

            void disconnect();

            [[nodiscard]] auto get_socket() const -> const CppSockets::TlsSocket & { return m_socket; }
            [[nodiscard]] auto get_socket() -> CppSockets::TlsSocket & { return m_socket; }

            [[nodiscard]] auto get_device_uuid() const -> std::string_view { return m_device_uuid; }
            [[nodiscard]] auto get_device_name() const -> std::string_view { return m_device_name; }
            [[nodiscard]] auto get_public_key() const -> std::string_view { return m_public_key; }

        protected:
            PeerBase(const CppSockets::IEndpoint &peer, CppSockets::TlsContext ctx = {});
            PeerBase(CppSockets::TlsSocket &&peer);

            virtual void authorize_request(Protocol::Request request) = 0;
            virtual auto parse_bytes(std::string_view raw_msg, Protocol::Request &out) -> std::size_t = 0;

            void poll_requests();

            auto get_buffer() -> std::string & { return m_buffer; }

            void set_device_uuid(std::string uuid) { m_device_uuid = std::move(uuid); }
            void set_device_name(std::string name) { m_device_name = std::move(name); }
            void set_public_key(std::string key) { m_public_key = std::move(key); }

            void read_peer_certificate();
        private:
            CppSockets::TlsSocket m_socket;

            std::string m_device_uuid;
            std::string m_device_name;
            std::string m_public_key;

            std::string m_buffer;
    };

    using PeerBase_ptr = std::shared_ptr<PeerBase>;
}
