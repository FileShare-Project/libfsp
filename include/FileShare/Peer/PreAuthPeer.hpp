/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Tue Jul 29 15:23:09 2025 Francois Michaut
** Last update Thu Aug 21 09:21:40 2025 Francois Michaut
**
** PreAuthPeer.hpp : Class to represent a Peer before it has been sucesfully Authenticated
*/

#pragma once

#include "FileShare/Peer/PeerBase.hpp"
#include "FileShare/Protocol/Protocol.hpp"

#include <memory>
#include <optional>

namespace FileShare {
    class PreAuthPeer : public PeerBase {
        public:
            enum Type {
                CLIENT,
                SERVER
            };

            PreAuthPeer(const CppSockets::IEndpoint &peer, Type type, CppSockets::TlsContext ctx = {});
            PreAuthPeer(CppSockets::TlsSocket &&peer, Type type);

            using PeerBase::poll_requests;

            void do_client_hello();

            [[nodiscard]] auto get_type() const -> Type { return m_type; }
            [[nodiscard]] auto get_protocol() const -> Protocol::Protocol;
            [[nodiscard]] auto has_protocol() const -> bool { return m_protocol.has_value(); }
        protected:
            auto parse_bytes(std::string_view raw_msg, Protocol::Request &out) -> std::size_t override;

        private:
            void authorize_request(Protocol::Request request) override;

            Type m_type;
            std::optional<Protocol::Protocol> m_protocol;
            bool m_first_extra_request = true;
    };

    using PreAuthPeer_ptr = std::shared_ptr<PreAuthPeer>;
}
