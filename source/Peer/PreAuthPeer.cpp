/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Thu Aug 14 12:00:55 2025 Francois Michaut
** Last update Thu Aug 21 09:21:57 2025 Francois Michaut
**
** PreAuthPeer.cpp : Implementation of the class to represent a Peer before it has been Authenticated
*/

#include "FileShare/Peer/PreAuthPeer.hpp"
#include "FileShare/Peer/PeerBase.hpp"
#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/Protocol/RequestData.hpp"
#include "FileShare/Protocol/Version.hpp"

#include <algorithm>
#include <ranges>
#include <stdexcept>

namespace FileShare {
    PreAuthPeer::PreAuthPeer(const CppSockets::IEndpoint &peer, Type type, CppSockets::TlsContext ctx) :
        PeerBase(peer, std::move(ctx)), m_type(type)
    {}

    PreAuthPeer::PreAuthPeer(CppSockets::TlsSocket &&peer, Type type) :
        PeerBase(std::move(peer)), m_type(type)
    {}

    void PreAuthPeer::do_client_hello() {
        get_socket().write(Protocol::IProtocolHandler::format_client_version_list());
    }

    auto PreAuthPeer::get_protocol() const -> Protocol::Protocol {
        if (m_protocol.has_value()) {
            return m_protocol.value();
        }
        throw std::runtime_error("PreAuthPeer hasn't negociated a Protocol yet");
    }

    auto PreAuthPeer::parse_bytes(std::string_view raw_msg, Protocol::Request &out) -> std::size_t {
        std::size_t ret;

        if (m_protocol.has_value()) {
            // In case the Peer sends the version and a command right after, we want to break the
            // poll_requests() loop. This should allow known peers or CLIENT peers to be promoted
            // before we reject the first requests as unauthorized. However, we give only 1 chance,
            // after that if the peer is still in PreAuth, we will reject the requests.
            if (m_first_extra_request) {
                m_first_extra_request = false;
                return 0;
            }

            return m_protocol.value().handler().parse_request(raw_msg, out);
        }

        try {
            if (m_type == CLIENT) {
                ret = Protocol::IProtocolHandler::parse_server_version(raw_msg, out);
            } else {
                ret = Protocol::IProtocolHandler::parse_client_version(raw_msg, out);
            }
        } catch (...) {
            this->get_socket().close(); // Invalid request -> Abort
        }

        return ret;
    }

    void PreAuthPeer::authorize_request(Protocol::Request request) {
        if (m_protocol.has_value()) {
            // If Peer is sending requests while still in PreAuth, deny them
            std::string message = m_protocol.value().handler().format_response(request.message_id, Protocol::StatusCode::UNAUTHORIZED);

            get_socket().write(message);
            return;
        }

        switch (request.code) {
            case Protocol::CommandCode::SUPPORTED_VERSIONS: {
                auto data = std::dynamic_pointer_cast<Protocol::SupportedVersionsData>(request.request);

                std::erase_if(data->versions, [](auto &version){
                    return !Protocol::Version::NAMES.contains(version);
                });
                if (data->versions.empty()) {
                    break; // No matching version -> Abort
                }

                Protocol::Version version = std::ranges::max(data->versions);

                m_protocol = Protocol::Protocol(version);
                get_socket().write(Protocol::IProtocolHandler::format_server_selected_version(version));
                return;
            }

            case Protocol::CommandCode::SELECTED_VERSION: {
                auto data = std::dynamic_pointer_cast<Protocol::SelectedVersionData>(request.request);

                if (!Protocol::Version::NAMES.contains(data->version)) {
                    break; // Abort, server is asking for a version we dont support
                }
                m_protocol = Protocol::Protocol(data->version);
                return;
            }
            default:
                break; // Passthrough
        }
        this->get_socket().close(); // Abort connection
    }
}
