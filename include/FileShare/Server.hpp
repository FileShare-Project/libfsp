/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 19:01:51 2022 Francois Michaut
** Last update Tue May  9 08:52:44 2023 Francois Michaut
**
** Server.hpp : Server part used to receive qnd process requests of Clients
*/

#pragma once

#include <vector>

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Config.hpp"
#include "FileShare/Client.hpp"

namespace FileShare {
    class Server {
        public:
            Server(std::shared_ptr<CppSockets::IEndpoint> server_endpoint = Server::default_endpoint(), Config config = Server::default_config());
            Server(Config config);

            // TODO: Server will handle the ProtocolVersion negotiation + Peer verification
            Client &connect(CppSockets::TlsSocket &&peer);
            Client &connect(const CppSockets::IEndpoint &peer);
            Client &connect(CppSockets::TlsSocket &&peer, const Config &config);
            Client &connect(const CppSockets::IEndpoint &peer, const Config &config);

            const Config &get_config() const;
            void set_config(const Config &config);
            static Config default_config();

            const CppSockets::TlsSocket &get_socket() const;

            void restart();
            bool disabled() const;
        private:
            void initialize_download_directory();
            void initialize_private_key();
            static std::shared_ptr<CppSockets::IEndpoint> default_endpoint();

            std::shared_ptr<CppSockets::IEndpoint> m_server_endpoint;
            CppSockets::TlsSocket m_socket;
            Config m_config;
            std::vector<Client> m_client_list;
    };
}
