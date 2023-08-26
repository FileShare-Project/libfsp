/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Thu Aug 24 19:36:36 2023 Francois Michaut
** Last update Sat Aug 26 19:52:33 2023 Francois Michaut
**
** TransferHandler.cpp : Implementation of classes to handle the file transfers
*/

#include "FileShare/TransferHandler.hpp"

#include <algorithm>

namespace FileShare {
    DownloadTransferHandler::DownloadTransferHandler(std::string destination_filename, std::shared_ptr<Protocol::SendFileData> original_request) :
        m_filename(std::move(destination_filename)), m_temp_filename(m_filename + ".fsdownload"), m_original_request(std::move(original_request))
    {
        if (std::filesystem::exists(m_temp_filename)) {
            // TODO: read from it and restart from where we left off
        } else if(std::filesystem::exists(m_filename) && Utils::file_hash(m_original_request->hash_algorithm, m_filename) == m_original_request->filehash) {
            // TODO: file is already up to date, don't need to download it again
            throw std::runtime_error("FILE IS UP TO DATE");
        } else {
            std::filesystem::create_directories(std::filesystem::path(m_temp_filename).parent_path());
            m_file.open(m_temp_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
        }
    }

    void DownloadTransferHandler::receive_packet(const Protocol::DataPacketData &data) {
        auto missing = std::find(m_missing_ids.begin(), m_missing_ids.end(), data.packet_id);

        if (missing != m_missing_ids.end())
            m_missing_ids.erase(missing);

        if (data.packet_id > m_expected_id) {
            std::size_t diff = data.packet_id - m_expected_id;

            m_missing_ids.reserve(m_missing_ids.size() + diff);
            for (std::size_t i = 0; i < diff; i++) {
                // Fill file with temporary 0s
                std::string str = std::string(m_original_request->packet_size, '\0');

                m_missing_ids.push_back(m_expected_id + i);
                m_file.write(str.data(), str.size());
            }
            m_expected_id = data.packet_id + 1;
        } else if (data.packet_id < m_expected_id) {
            auto original_pos = m_file.tellp();

            // Go back to the skipped position, write the data, and go back to the current position
            m_file.seekp(m_original_request->packet_size * data.packet_id);
            m_file.write(data.data.data(), data.data.size());
            m_file.seekp(original_pos);
            if (m_missing_ids.empty() && m_expected_id == m_original_request->total_packets) {
                finish_transfer();
            }
        } else { // data.packet_id == m_expected_id
            bool last_packet;

            m_expected_id++; // Increment before comparaison, cause if we need 2 packets, we will receive ids 0 and 1.
            last_packet = m_expected_id == m_original_request->total_packets;

            m_file.write(data.data.data(), data.data.size());
            if (data.data.size() != m_original_request->packet_size && !last_packet) {
                // TODO: something is wrong if this happens -> figure out what to do.
                throw std::runtime_error("Transfert size invalid");
            } else if (last_packet && m_missing_ids.empty()) {
                finish_transfer();
            }
        }
    }

    void DownloadTransferHandler::finish_transfer() {
        m_file.close();
        if (Utils::file_hash(m_original_request->hash_algorithm, m_temp_filename) == m_original_request->filehash) {
            std::filesystem::rename(m_temp_filename, m_filename);
        } else {
            // TODO: figure out what to do
            throw std::runtime_error("transferred file hash missmatch");
        }
    }

    bool DownloadTransferHandler::finished() const {
        return !m_file.is_open();
    }

    std::size_t DownloadTransferHandler::get_current_size() const {
        // TODO: this is not exact : last packet might have a smaller size
        return m_expected_id * m_original_request->packet_size;
    }

    std::size_t DownloadTransferHandler::get_total_size() const {
        // TODO: this is not exact : last packet might have a smaller size
        return m_original_request->total_packets * m_original_request->packet_size;
    }
}
