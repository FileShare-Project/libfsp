/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Thu Aug 24 08:51:14 2023 Francois Michaut
** Last update Sat Aug 23 11:26:19 2025 Francois Michaut
**
** TransferHandler.hpp : Classes to handle the file transfers
*/

#include "FileShare/Config/FileMapping.hpp"
#include "FileShare/Protocol/RequestData.hpp"

#include <fstream>

namespace FileShare {
    class ITransferHandler {
        public:
            virtual ~ITransferHandler() = default;

            ITransferHandler() = default;

            ITransferHandler(const ITransferHandler &) = default;
            ITransferHandler(ITransferHandler &&) = default;
            auto operator=(const ITransferHandler &) -> ITransferHandler & = default;
            auto operator=(ITransferHandler &&) -> ITransferHandler & = default;

            [[nodiscard]] virtual auto finished() const -> bool = 0;
    };

    class IFileTransferHandler : public ITransferHandler {
        public:
            [[nodiscard]] auto get_current_size() const -> std::size_t;
            [[nodiscard]] auto get_total_size() const -> std::size_t;
            [[nodiscard]] auto get_original_request() const -> std::shared_ptr<Protocol::SendFileData>;
        protected:
            std::size_t m_transferred_size = 0;
            std::shared_ptr<Protocol::SendFileData> m_original_request;
    };

    class DownloadTransferHandler : public IFileTransferHandler {
        public:
            DownloadTransferHandler(std::string destination_filename, std::shared_ptr<Protocol::SendFileData> original_request);
            ~DownloadTransferHandler() override = default;

            void receive_packet(const Protocol::DataPacketData &data);

            bool m_keep = false; // TODO HACK: find a REAL solution

            auto finished() const -> bool override;
        private:
            void finish_transfer();

            std::string m_filename;

            std::string m_temp_filename;
            std::vector<std::size_t> m_missing_ids;
            std::size_t m_expected_id = 0;
            std::ofstream m_file;
    };

    class UploadTransferHandler : public IFileTransferHandler {
        public:
            UploadTransferHandler(const std::string &filepath, std::shared_ptr<Protocol::SendFileData> original_request, std::size_t packet_start);
            UploadTransferHandler(UploadTransferHandler &&other) noexcept = default;
            ~UploadTransferHandler() override = default;

            auto operator=(UploadTransferHandler &&other) noexcept -> UploadTransferHandler & = default;

            auto get_next_packet(Protocol::MessageID original_request_id) -> std::shared_ptr<Protocol::DataPacketData>;

            auto finished() const -> bool override;
        private:
            std::size_t m_packet_id = 0;
            std::ifstream m_file;
    };

    class ListFilesTransferHandler : public ITransferHandler {
        public:
            ListFilesTransferHandler(std::filesystem::path requested_path, FileMapping &file_mapping, std::size_t packet_size);
            ~ListFilesTransferHandler() override = default;

            auto get_next_packet(Protocol::MessageID original_request_id) -> std::shared_ptr<Protocol::FileListData>;

            [[nodiscard]] auto finished() const -> bool override;
        private:
            std::filesystem::path m_requested_path;
            FileMapping &m_file_mapping;
            std::optional<PathNode> m_path_node;

            std::filesystem::directory_iterator m_directory_iterator;
            PathNode::NodeMap::const_iterator m_node_iterator;

            std::size_t m_packet_size;
            std::size_t m_current_id = 0;
            bool m_extra_packet_sent = false;
    };

    class FileListTransferHandler : public ITransferHandler {
        public:
            FileListTransferHandler() = default;
            ~FileListTransferHandler() override = default;

            void receive_packet(Protocol::FileListData data);

            [[nodiscard]] auto finished() const -> bool override;
            [[nodiscard]] auto get_file_list() const -> const std::vector<Protocol::FileInfo> & { return m_file_list; }
        private:
            std::vector<Protocol::FileInfo> m_file_list;

            std::size_t m_current_id = 0;
            std::vector<std::size_t> m_missing_ids;
            bool m_finished;
    };
};
