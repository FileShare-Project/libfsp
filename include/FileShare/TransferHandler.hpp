/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Thu Aug 24 08:51:14 2023 Francois Michaut
** Last update Sat Dec  9 08:49:52 2023 Francois Michaut
**
** TransferHandler.hpp : Classes to handle the file transfers
*/

#include "FileShare/Config.hpp"
#include "FileShare/Protocol/RequestData.hpp"

#include <fstream>

namespace FileShare {
    class ITransferHandler {
        public:
            virtual bool finished() const = 0;
    };

    class IFileTransferHandler : public ITransferHandler {
        public:
            std::size_t get_current_size() const;
            std::size_t get_total_size() const;
            std::shared_ptr<Protocol::SendFileData> get_original_request() const;
        protected:
            std::size_t m_transferred_size = 0;
            std::shared_ptr<Protocol::SendFileData> m_original_request;
    };

    class DownloadTransferHandler : public IFileTransferHandler {
        public:
            DownloadTransferHandler(std::string destination_filename, std::shared_ptr<Protocol::SendFileData> original_request);
            virtual ~DownloadTransferHandler() = default;

            void receive_packet(const Protocol::DataPacketData &data);

            bool finished() const override;
        private:
            void finish_transfer();

        private:
            std::string m_filename;

            std::string m_temp_filename;
            std::vector<std::size_t> m_missing_ids;
            std::size_t m_expected_id = 0;
            std::ofstream m_file;
    };

    class UploadTransferHandler : public IFileTransferHandler {
        public:
            UploadTransferHandler(std::string filepath, std::shared_ptr<Protocol::SendFileData> original_request, std::size_t packet_start);
            UploadTransferHandler(UploadTransferHandler &&other) noexcept = default;
            virtual ~UploadTransferHandler() = default;

            UploadTransferHandler &operator=(UploadTransferHandler &&other) noexcept = default;

            std::shared_ptr<Protocol::DataPacketData> get_next_packet(Protocol::MessageID original_request_id);

            bool finished() const override;
        private:
            std::size_t m_packet_id = 0;
            std::ifstream m_file;
    };

    class ListFilesTransferHandler : public ITransferHandler {
        public:
            ListFilesTransferHandler(std::filesystem::path requested_path, FileMapping &file_mapping, std::size_t packet_size);
            virtual ~ListFilesTransferHandler() = default;

            std::shared_ptr<Protocol::FileListData> get_next_packet(Protocol::MessageID original_request_id);

            bool finished() const override;
        private:
            std::filesystem::path m_requested_path;
            FileMapping &m_file_mapping;
            std::optional<PathNode> m_path_node;

            std::filesystem::directory_iterator m_directory_iterator;
            PathNode::NodeMap::iterator m_node_iterator;

            std::size_t m_packet_size;
            std::size_t m_current_id = 0;
            bool m_extra_packet_sent = false;
    };

    class FileListTransferHandler : public ITransferHandler {
        public:
            FileListTransferHandler() = default;
            virtual ~FileListTransferHandler() = default;

            void receive_packet(Protocol::FileListData data);

            bool finished() const override;
            [[nodiscard]] const std::vector<Protocol::FileInfo> &get_file_list() const;
        private:
            std::vector<Protocol::FileInfo> m_file_list;

            std::size_t m_current_id = 0;
            std::vector<std::size_t> m_missing_ids;
            bool m_finished;
    };
};
