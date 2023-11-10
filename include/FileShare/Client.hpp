/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:23:07 2022 Francois Michaut
** Last update Sat Oct 28 11:15:11 2023 Francois Michaut
**
** Client.hpp : Client to communicate with peers with the FileShareProtocol
*/

#pragma once

#include "FileShare/Config.hpp"
#include "FileShare/Protocol/Protocol.hpp"
#include "FileShare/MessageQueue.hpp"
#include "FileShare/TransferHandler.hpp"

#include <CppSockets/IPv4.hpp>
#include <CppSockets/TlsSocket.hpp>
#include <CppSockets/Version.hpp>

#include <functional>

// TODO handle UDP
// TODO: rename Client -> Peer
namespace FileShare {
    class Client {
        public:
            using ProgressCallback = std::function<void(const std::string &filepath, std::size_t current_size, std::size_t total_size)>;

            Client(const CppSockets::IEndpoint &peer, std::string device_uuid, std::string public_key, Protocol::Protocol protocol, Config config = Client::default_config());
            Client(CppSockets::TlsSocket &&peer, std::string device_uuid, std::string public_key, Protocol::Protocol protocol, Config config = Client::default_config());

            [[nodiscard]]
            std::vector<Protocol::Request> pull_requests();
            void respond_to_request(Protocol::Request, Protocol::StatusCode);

            // Blocking functions
            Protocol::Response<void> send_file(std::string filepath, ProgressCallback progress_callback = [](const std::string &filepath, std::size_t current_size, std::size_t total_size){});
            Protocol::Response<void> receive_file(std::string filepath, ProgressCallback progress_callback = [](const std::string &filepath, std::size_t current_size, std::size_t total_size){});
            Protocol::Response<Protocol::FileList> list_files(std::string folderpath = "", std::size_t page_nb = 0);

            // TODO determine params
            Protocol::Response<void> initiate_pairing();
            Protocol::Response<void> accept_pairing();

            // TODO: Async non-blocking Functions

            [[nodiscard]]
            const CppSockets::TlsSocket &get_socket() const;
            void reconnect(const CppSockets::IEndpoint &peer);
            void reconnect(CppSockets::TlsSocket &&peer);

            [[nodiscard]]
            const Config &get_config() const;
            void set_config(Config config);

            [[nodiscard]]
            const Protocol::Protocol &get_protocol() const;
            void set_protocol(Protocol::Protocol protocol);

            [[nodiscard]]
            std::string_view get_device_uuid() const;
            [[nodiscard]]
            std::string_view get_public_key() const;
            void set_config(std::string device_uuid);
            void set_public_key(std::string public_key);

        protected:
            static Config default_config();
            using UploadTransferMap = std::unordered_map<Protocol::MessageID, UploadTransferHandler>;
            using DownloadTransferMap = std::unordered_map<Protocol::MessageID, DownloadTransferHandler>;

        private:
            Protocol::StatusCode wait_for_status(Protocol::MessageID message_id);
            void poll_requests();

            void authorize_request(Protocol::Request request);

            void receive_reply(Protocol::MessageID message_id, Protocol::StatusCode status);
            void send_reply(Protocol::MessageID message_id, Protocol::StatusCode status);
            std::uint8_t send_request(Protocol::CommandCode command, std::shared_ptr<Protocol::IRequestData> request_data);
            std::uint8_t send_request(Protocol::Request request);
            UploadTransferMap::iterator create_upload(std::string filepath, std::size_t packet_size, std::size_t packet_start);
            UploadTransferMap::iterator create_upload(std::string filepath);
            DownloadTransferMap::iterator create_download(Protocol::MessageID message_id, const std::shared_ptr<Protocol::SendFileData> &data);

        private:
            CppSockets::TlsSocket m_socket;
            Config m_config;
            Protocol::Protocol m_protocol;
            std::string m_device_uuid;
            std::string m_public_key;

            std::string m_buffer;
            std::vector<Protocol::Request> m_request_buffer;
            DownloadTransferMap m_download_transfers;
            UploadTransferMap m_upload_transfers;
            MessageQueue m_message_queue;
    };
}
