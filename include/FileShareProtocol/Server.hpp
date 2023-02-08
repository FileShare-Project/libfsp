/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 19:01:51 2022 Francois Michaut
** Last update Tue Feb  7 22:39:22 2023 Francois Michaut
**
** Server.hpp : Server part used to receive qnd process requests of Clients
*/

#pragma once

#include <vector>

#include "FileShareProtocol/Definitions.hpp"
#include "FileShareProtocol/Config.hpp"
#include "FileShareProtocol/Client.hpp"

namespace FileShareProtocol {
    class Server {
        public:
            Server(Config config = Server::default_config());

            Client &connect(CppSockets::TlsSocket &&peer);
            Client &connect(const CppSockets::IEndpoint &peer);
            Client &connect(CppSockets::TlsSocket &&peer, const Config &config);
            Client &connect(const CppSockets::IEndpoint &peer, const Config &config);

            const Config &get_config() const;
            void set_config(const Config &config);
            static Config default_config();

            const CppSockets::TlsSocket &get_socket() const;

        private:
            void initialize_download_directory();
            void initialize_private_key();

            Config m_config;
            std::vector<Client> m_client_list;
            CppSockets::TlsSocket m_socket;
    };
}
