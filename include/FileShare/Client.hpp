/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:23:07 2022 Francois Michaut
** Last update Tue May  9 08:53:30 2023 Francois Michaut
**
** Client.hpp : Client to communicate with peers with the FileShareProtocol
*/

#pragma once

#include "FileShare/Config.hpp"
#include "FileShare/Protocol/Definitions.hpp"

#include <CppSockets/IPv4.hpp>
#include <CppSockets/TlsSocket.hpp>
#include <CppSockets/Version.hpp>

#include <functional>

// TODO handle UDP
// TODO: rename Client -> Peer
namespace FileShare {
    class Client {
        public:
            using ProgressCallback = std::function<void(const std::string &filepath, float percentage, std::size_t current_size, std::size_t total_size)>;

            explicit Client(const CppSockets::IEndpoint &peer, Config config = Client::default_config());
            explicit Client(CppSockets::TlsSocket &&peer, Config config = Client::default_config());

            [[nodiscard]]
            const Config &get_config() const;
            void set_config(const Config &config);

            [[nodiscard]]
            const CppSockets::TlsSocket &get_socket() const;
            void reconnect(const CppSockets::IEndpoint &peer);
            void reconnect(CppSockets::TlsSocket &&peer);

            // Blocking functions
            Protocol::Response<void> send_file(std::string filepath, ProgressCallback progress_callback = {});
            Protocol::Response<void> receive_file(std::string filepath, ProgressCallback progress_callback = {});
            Protocol::Response<Protocol::FileList> list_files(std::string folderpath = "", std::size_t page_idx = 0);

            // TODO determine params
            Protocol::Response<void> initiate_pairing();
            Protocol::Response<void> accept_pairing();

            // TODO: Async non-blocking Functions
        protected:
            static Config default_config();
        private:
            CppSockets::TlsSocket socket;
            Config config;
    };
}
