/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:23:07 2022 Francois Michaut
** Last update Wed Sep 14 22:25:37 2022 Francois Michaut
**
** Client.hpp : Client to communicate with peers with the FileShareProtocol
*/

#pragma once

#include "FileShareProtocol/ClientConfig.hpp"
#include "FileShareProtocol/Definitions.hpp"

#include <CppSockets/IPv4.hpp>
#include <CppSockets/Socket.hpp>
#include <CppSockets/Version.hpp>

#include <functional>

namespace FileShareProtocol {
    class Client {
        public:
            using ProgressCallback = std::function<void(const std::string &filepath, float percentage, std::size_t current_size, std::size_t total_size)>;

            Client(const CppSockets::IEndpoint &peer, ClientConfig config = ClientConfig::get_default());
            Client(CppSockets::Socket &&peer, ClientConfig config = ClientConfig::get_default());

            [[nodiscard]]
            const ClientConfig &get_config() const;
            void set_config(const ClientConfig &config);

            [[nodiscard]]
            const CppSockets::Socket &get_socket() const;
            void reconnect(const CppSockets::IEndpoint &peer);
            void reconnect(CppSockets::Socket &&peer);

            Response<void> send_file(std::string filepath, ProgressCallback progress_callback = {});
            Response<void> receive_file(std::string filepath, ProgressCallback progress_callback = {});
            Response<FileList> list_files(std::size_t page_idx = 0, std::string folderpath = "");

            // TODO determine params
            Response<void> initiate_pairing();
            Response<void> accept_pairing();
        private:
            CppSockets::Socket socket;
            ClientConfig config;
    };
}
