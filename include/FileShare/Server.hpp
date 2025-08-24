/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 19:01:51 2022 Francois Michaut
** Last update Fri Aug 22 00:39:26 2025 Francois Michaut
**
** Server.hpp : Server part used to receive qnd process requests of Peers
*/

#pragma once

#include "FileShare/Config/Config.hpp"
#include "FileShare/Config/KnownPeerStore.hpp"
#include "FileShare/Config/ServerConfig.hpp"
#include "FileShare/Peer/Peer.hpp"
#include "FileShare/Peer/PreAuthPeer.hpp"
#include "FileShare/Protocol/Definitions.hpp"

#include <CppSockets/Socket.hpp>
#include <CppSockets/Tls/Context.hpp>
#include <CppSockets/Tls/Socket.hpp>
#include <CppSockets/Tls/Utils.hpp>

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

struct pollfd;

namespace FileShare {
    class Server {
        public:
            class Event {
                public:
                    enum Type {
                        NONE,
                        CONNECT,
                        REQUEST
                    };

                    Event(Type, PeerBase_ptr, std::optional<Protocol::Request> = {});
                    Event() = default;
                    ~Event() = default;

                    Event(const Event &) = default;
                    Event(Event &&) = default;
                    auto operator=(const Event &) -> Event & = default;
                    auto operator=(Event &&) -> Event & = default;

                    auto type() -> Type & { return m_type; }
                    auto peer() -> PeerBase_ptr & { return m_peer; }
                    auto request() -> std::optional<Protocol::Request> & { return m_request; }
                private:
                    Type m_type = NONE;
                    PeerBase_ptr m_peer;
                    std::optional<Protocol::Request> m_request;
            };

            using PeerAcceptCallback = std::function<bool(Server &, PreAuthPeer_ptr &peer)>;
            using PeerRequestCallback = std::function<void(
                Server &, Peer_ptr &peer, Protocol::Request &req
            )>;
            using PeerRequestEventCallback = std::function<void(Server &, Event)>;

            using PeerMap = std::unordered_map<RawSocketType, Peer_ptr>;
            using PreAuthPeerMap = std::unordered_map<RawSocketType, PreAuthPeer_ptr>;

            using FdVector = std::vector<struct pollfd>;

            Server(
                std::shared_ptr<CppSockets::IEndpoint> server_endpoint = Server::default_endpoint(),
                ServerConfig config = Server::default_config(),
                Config peer_config = Server::default_peer_config()
            );
            Server(ServerConfig config, Config peer_config = Server::default_peer_config());

            ~Server();

            // TODO: Allow copy ? What would that even mean ?
            Server(const Server &) = delete;
            Server(Server &&) = default;
            auto operator=(const Server &) -> Server & = delete;
            auto operator=(Server &&) -> Server & = default;

             // Call one of theses in a loop in your main program !
             // Otherwise server won't accept incomming connections or process
             // incomming/outgoing messages.
            void process_events(const PeerAcceptCallback &accept_cb, const PeerRequestCallback &request_cb);
            void process_events(const PeerAcceptCallback &accept_cb, const PeerRequestEventCallback &request_cb);
            auto pull_event(Event &result) -> bool; // TODO: figure out how to accept commands here

            // TODO: Server will handle the ProtocolVersion negotiation + Peer verification
            auto connect(CppSockets::TlsSocket peer) -> Peer_ptr & { return connect(std::move(peer), this->m_peer_config); }
            auto connect(const CppSockets::IEndpoint &peer) -> Peer_ptr & { return connect(peer, this->m_peer_config); }
            auto connect(CppSockets::TlsSocket peer, const Config &config) -> Peer_ptr &;
            auto connect(const CppSockets::IEndpoint &peer, const Config &config) -> Peer_ptr &;

            void accept_peer(PreAuthPeer_ptr peer, bool temporary_trust = false);

            auto get_config() -> ServerConfig & { return m_config; }
            auto get_config() const -> const ServerConfig & { return m_config; }
            void set_config(const ServerConfig &config) { m_config = config; }

            // Warning : changing the default peer configuration does NOT
            // change the already connected Peers, only new ones. You need to
            // manually update the configuration of each existing peer.
            auto get_peer_config() -> Config & { return m_peer_config; }
            auto get_peer_config() const -> const Config & { return m_peer_config; }
            void set_peer_config(const Config &config) { m_peer_config = config; }

            static auto default_config() -> ServerConfig;
            static auto default_peer_config() -> Config;

            auto get_server_endpoint() const -> const CppSockets::IEndpoint & { return *m_server_endpoint; }
            auto get_socket() const -> const CppSockets::TlsSocket & { return m_socket; }

            auto get_peers() const -> const PeerMap & { return m_peers; }
            auto get_pending_peers() const -> const PreAuthPeerMap & { return m_pending_authorization_peers; }

            auto get_poll_fds() const -> const FdVector & { return m_fds; }

            void restart();
            auto disabled() const -> bool { return m_config.is_server_disabled(); }
            void set_disabled(bool disabled);
        private:
            void initialize_private_key();
            void poll_events();

            auto handle_peer_events(FdVector::iterator iter) -> FdVector::iterator;
            auto delete_peer(FdVector::iterator iter) -> FdVector::iterator;
            auto delete_pre_auth_peer(FdVector::iterator iter, PreAuthPeerMap &map) -> FdVector::iterator;
            auto insert_peer(PreAuthPeer_ptr &&peer) -> Peer_ptr &;
            auto insert_peer(Peer_ptr peer) -> Peer_ptr &;

            static auto default_endpoint() -> std::shared_ptr<CppSockets::IEndpoint>;

            std::shared_ptr<CppSockets::IEndpoint> m_server_endpoint;
            CppSockets::TlsContext m_ctx;
            CppSockets::TlsSocket m_socket;
            ServerConfig m_config;
            Config m_peer_config;

            // TODO: Move the Peers management to a different class
            KnownPeerStore m_known_peers;
            PreAuthPeerMap m_handshake_peers;
            PreAuthPeerMap m_pending_authorization_peers;
            PeerMap m_peers;

            FdVector m_fds;
            std::vector<Event> m_events;
    };
}
