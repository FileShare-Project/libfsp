/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Thu Aug 24 08:51:14 2023 Francois Michaut
** Last update Sat Aug 26 16:56:23 2023 Francois Michaut
**
** TransferHandler.hpp : Classes to handle the file transfers
*/

#include "FileShare/Config.hpp"
#include "FileShare/Protocol/RequestData.hpp"

#include <fstream>

namespace FileShare {
    class DownloadTransferHandler {
        public:
            DownloadTransferHandler(std::string destination_filename, std::shared_ptr<Protocol::SendFileData> original_request);
            DownloadTransferHandler(DownloadTransferHandler &&other);

            void receive_packet(const Protocol::DataPacketData &data);

            bool finished() const;
            std::size_t get_current_size() const;
            std::size_t get_total_size() const;
        private:
            void finish_transfer();
        private:
            std::string m_filename;
            std::string m_temp_filename;
            std::shared_ptr<Protocol::SendFileData> m_original_request;
            std::ofstream m_file;
            std::vector<std::size_t> m_missing_ids;
            std::size_t m_expected_id = 0;
    };

    class UploadTransferHandler {
        public:
            UploadTransferHandler(std::string filepath, Utils::HashAlgorithm hash_algo, std::size_t packet_size);

            std::size_t get_current_size() const;
            std::size_t get_total_size() const;
        private:
    };
};
