/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 19:01:51 2022 Francois Michaut
** Last update Tue Jul 18 13:47:11 2023 Francois Michaut
**
** Server.hpp : Server part used to receive qnd process requests of Clients
*/

#pragma once

#include <optional>
#include <vector>

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Config.hpp"
#include "FileShare/Client.hpp"

namespace FileShare {
    class Server {
        public:
            using ClientAcceptCallback = std::function<bool(Server &, Client &client)>;
            using ClientRequestCallback = std::function<void(Server &, Client &peer, Protocol::Request req)>;
            using Event = std::tuple<Client, std::optional<Protocol::Request>>;

            Server(std::shared_ptr<CppSockets::IEndpoint> server_endpoint = Server::default_endpoint(), Config config = Server::default_config());
            Server(Config config);

            // TODO: Server will handle the ProtocolVersion negotiation + Peer verification
            Client &connect(CppSockets::TlsSocket peer);
            Client &connect(const CppSockets::IEndpoint &peer);
            Client &connect(CppSockets::TlsSocket peer, const Config &config);
            Client &connect(const CppSockets::IEndpoint &peer, const Config &config);

            // Warning : changing the server configuration does NOT change the
            // already connected Clients, only new ones. You need to manually update
            // the configuration of each client.
            Config &get_config();
            const Config &get_config() const;
            void set_config(const Config &config);
            static Config default_config();

            const CppSockets::TlsSocket &get_socket() const;

             // Call one of theses in a loop in your main program !
             // Otherwise server won't accept incomming connections or process
             // incomming/outgoing messages.
            void process_events(ClientAcceptCallback accept_cb, ClientRequestCallback request_cb);
            bool pull_event(Event &result);

            std::vector<Client> &get_clients();
            const std::vector<Client> &get_clients() const;

            void restart();
            bool disabled() const;
        private:
            void initialize_download_directory();
            void initialize_private_key();
            void poll_events();

            static std::shared_ptr<CppSockets::IEndpoint> default_endpoint();

            std::shared_ptr<CppSockets::IEndpoint> m_server_endpoint;
            CppSockets::TlsSocket m_socket;
            Config m_config;
            std::vector<Client> m_client_list;
            std::vector<Event> m_events;
    };
}
