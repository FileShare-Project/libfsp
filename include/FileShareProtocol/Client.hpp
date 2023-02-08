/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:23:07 2022 Francois Michaut
** Last update Tue Feb  7 22:40:26 2023 Francois Michaut
**
** Client.hpp : Client to communicate with peers with the FileShareProtocol
*/

#pragma once

#include "FileShareProtocol/Config.hpp"
#include "FileShareProtocol/Definitions.hpp"

#include <CppSockets/IPv4.hpp>
#include <CppSockets/TlsSocket.hpp>
#include <CppSockets/Version.hpp>

#include <functional>

// TODO handle UDP
namespace FileShareProtocol {
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

            Response<void> send_file(std::string filepath, ProgressCallback progress_callback = {});
            Response<void> receive_file(std::string filepath, ProgressCallback progress_callback = {});
            Response<FileList> list_files(std::size_t page_idx = 0, std::string folderpath = "");

            // TODO determine params
            Response<void> initiate_pairing();
            Response<void> accept_pairing();
        protected:
            static Config default_config();
        private:
            CppSockets::TlsSocket socket;
            Config config;
    };
}
