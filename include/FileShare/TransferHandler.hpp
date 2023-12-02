/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Thu Aug 24 08:51:14 2023 Francois Michaut
** Last update Tue Nov 28 23:05:41 2023 Francois Michaut
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

            std::size_t get_current_size() const;
            std::size_t get_total_size() const;
            std::shared_ptr<Protocol::SendFileData> get_original_request() const;
        protected:
            std::shared_ptr<Protocol::SendFileData> m_original_request;

            std::size_t m_transferred_size = 0;
    };

    class DownloadTransferHandler : public ITransferHandler {
        public:
            DownloadTransferHandler(std::string destination_filename, std::shared_ptr<Protocol::SendFileData> original_request);
            DownloadTransferHandler(DownloadTransferHandler &&other);

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

    class UploadTransferHandler : public ITransferHandler {
        public:
            UploadTransferHandler(std::string filepath, std::shared_ptr<Protocol::SendFileData> original_request, std::size_t packet_start);

            std::shared_ptr<Protocol::DataPacketData> get_next_packet(Protocol::MessageID original_request_id);

            bool finished() const override;
        private:
            std::size_t m_packet_id = 0;
            std::ifstream m_file;
    };
};
