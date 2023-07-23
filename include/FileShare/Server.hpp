/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 19:01:51 2022 Francois Michaut
** Last update Sat Jul 22 21:58:00 2023 Francois Michaut
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
            using ClientAcceptCallback = std::function<bool(Server &, std::shared_ptr<Client> &client)>;
            using ClientRequestCallback = std::function<void(Server &, std::shared_ptr<Client> &peer, Protocol::Request &req)>;

            // TODO: find a better way for this
            class Event {
                public:
                    Event() = default;
                    Event(std::shared_ptr<Client>, std::optional<Protocol::Request>);

                    std::shared_ptr<Client> &client();
                    std::optional<Protocol::Request> &request();
                private:
                    std::shared_ptr<Client> m_client;
                    std::optional<Protocol::Request> m_request;
            };

            Server(std::shared_ptr<CppSockets::IEndpoint> server_endpoint = Server::default_endpoint(), Config config = Server::default_config());
            Server(Config config);

            // TODO: Server will handle the ProtocolVersion negotiation + Peer verification
            std::shared_ptr<Client> &connect(CppSockets::TlsSocket peer);
            std::shared_ptr<Client> &connect(const CppSockets::IEndpoint &peer);
            std::shared_ptr<Client> &connect(CppSockets::TlsSocket peer, const Config &config);
            std::shared_ptr<Client> &connect(const CppSockets::IEndpoint &peer, const Config &config);

            void accept_client(std::shared_ptr<Client> peer);

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

            std::map<RawSocketType, std::shared_ptr<Client>> &get_clients();
            const std::map<RawSocketType, std::shared_ptr<Client>> &get_clients() const;

            void restart();
            bool disabled() const;
        private:
            void initialize_download_directory();
            void initialize_private_key();
            void poll_events();
            bool handle_client_events(std::shared_ptr<Client> &client);
            std::shared_ptr<Client> &insert_client(std::shared_ptr<Client> client);

            static std::shared_ptr<CppSockets::IEndpoint> default_endpoint();

            std::shared_ptr<CppSockets::IEndpoint> m_server_endpoint;
            CppSockets::TlsSocket m_socket;
            Config m_config;
            std::map<RawSocketType, std::shared_ptr<Client>> m_clients;
            std::vector<Event> m_events;
    };
}
