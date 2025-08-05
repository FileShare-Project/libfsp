/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:23:07 2022 Francois Michaut
** Last update Sat Aug 23 00:08:01 2025 Francois Michaut
**
** Peer.hpp : Client to communicate with peers using the FileShareProtocol
*/

#pragma once

#include "FileShare/Config/Config.hpp"
#include "FileShare/MessageQueue.hpp"
#include "FileShare/Peer/PeerBase.hpp"
#include "FileShare/Peer/PreAuthPeer.hpp"
#include "FileShare/TransferHandler.hpp"

#include <CppSockets/IPv4.hpp>
#include <CppSockets/Tls/Socket.hpp>
#include <CppSockets/Version.hpp>

#include <functional>
#include <memory>

// TODO handle UDP
namespace FileShare {
    class Peer : public PeerBase {
        public:
            using ProgressCallback = std::function<void( const std::string &filepath, std::size_t current_size, std::size_t total_size)>;

            Peer(PreAuthPeer &&peer, Config config = Peer::default_config());

            // TODO: Allow copy ? What would that even mean ?
            Peer(const Peer &) = delete;
            Peer(Peer &&) = default;
            auto operator=(const Peer &) -> Peer & = delete;
            auto operator=(Peer &&) -> Peer & = default;

            ~Peer() override = default;

            // Call respond_to_request to answer to a Request Event
            void respond_to_request(Protocol::Request request, Protocol::StatusCode status);
            [[nodiscard]] auto pull_requests() -> std::vector<Protocol::Request>;

            // Blocking functions
            auto send_file(const std::string &filepath, const ProgressCallback &progress_callback = [](const std::string &, std::size_t, std::size_t) {}) -> Protocol::Response<void>;
            auto receive_file(std::string filepath, const ProgressCallback &progress_callback = [](const std::string &, std::size_t, std::size_t) {}) -> Protocol::Response<void>;
            auto list_files(std::string folderpath = "") -> Protocol::Response<std::vector<Protocol::FileInfo>>;

            // TODO: Async functions
            auto send_file_async(std::string filepath, const ProgressCallback &progress_callback = [](const std::string &, std::size_t, std::size_t) {}) -> Protocol::Response<void>;
            auto receive_file_async(std::string filepath, const ProgressCallback &progress_callback = [](const std::string &, std::size_t, std::size_t) {}) -> Protocol::Response<void>;
            auto list_files_async(std::string folderpath = "") -> Protocol::Response<std::vector<Protocol::FileInfo>>;

            // TODO determine params
            auto initiate_pairing() -> Protocol::Response<void>;
            auto accept_pairing() -> Protocol::Response<void>;

            [[nodiscard]] auto get_config() const -> const Config & { return m_config; }
            [[nodiscard]] auto get_config() -> Config & { return m_config; }
            void set_config(Config config) { m_config = std::move(config); }

        private:
            using UploadTransferMap = std::unordered_map<Protocol::MessageID, UploadTransferHandler>;
            using DownloadTransferMap = std::unordered_map<Protocol::MessageID, DownloadTransferHandler>;
            using ListFilesTransferMap = std::unordered_map<Protocol::MessageID, ListFilesTransferHandler>;
            using FileListTransferMap = std::unordered_map<Protocol::MessageID, FileListTransferHandler>;

            // TODO: Remove
            [[deprecated]] auto wait_for_status(Protocol::MessageID message_id) -> Protocol::StatusCode;

            void send_reply(Protocol::MessageID message_id, Protocol::StatusCode status);
            auto send_request(Protocol::CommandCode command, std::shared_ptr<Protocol::IRequestData> request_data) -> std::uint8_t;
            auto send_request(Protocol::Request request) -> std::uint8_t;

            void authorize_request(Protocol::Request request) override;
            auto parse_bytes(std::string_view raw_msg, Protocol::Request &out) -> std::size_t override;

            // TODO: Rename to handle_reply / process_reply / dispatch_reply ?
            void receive_reply(Protocol::MessageID message_id, Protocol::StatusCode status);

            auto prepare_upload(std::filesystem::path host_filepath, std::string virtual_filepath, std::size_t packet_size, std::size_t packet_start) -> std::pair<std::optional<UploadTransferHandler>, Protocol::StatusCode>;
            auto create_host_upload(std::filesystem::path host_filepath) -> UploadTransferMap::iterator;
            auto create_upload(UploadTransferHandler handler) -> UploadTransferMap::iterator;
            auto create_download(Protocol::MessageID request_id, const std::shared_ptr<Protocol::SendFileData> &data) -> DownloadTransferMap::iterator;

        protected:
            static auto default_config() -> Config;

        private:
            Config m_config;

            Protocol::Protocol m_protocol;
            std::vector<Protocol::Request> m_request_buffer;
            MessageQueue m_message_queue;

            DownloadTransferMap m_download_transfers;
            UploadTransferMap m_upload_transfers;
            ListFilesTransferMap m_list_files_transfers;
            FileListTransferMap m_file_list_transfers;
    };

    using Peer_ptr = std::shared_ptr<Peer>;
}
