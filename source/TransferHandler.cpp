/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Thu Aug 24 19:36:36 2023 Francois Michaut
** Last update Sat Aug 23 11:16:50 2025 Francois Michaut
**
** TransferHandler.cpp : Implementation of classes to handle the file transfers
*/

#include "FileShare/Errors/TransferErrors.hpp"
#include "FileShare/TransferHandler.hpp"

#include <algorithm>

namespace FileShare {
    // TODO: make download transfer handler return a STATUS instead.
    // Can return status::up_to_date for instance, avoid handling this with exceptions
    DownloadTransferHandler::DownloadTransferHandler(std::string destination_filename, std::shared_ptr<Protocol::SendFileData> original_request) :
        m_filename(std::move(destination_filename)), m_temp_filename(m_filename + ".fsdownload")
    {
        m_original_request = std::move(original_request);
        if (std::filesystem::exists(m_temp_filename)) {
            // TODO: read from it and restart from where we left off
            throw Errors::Transfer::UpToDateError(m_temp_filename);
        }
        if(std::filesystem::exists(m_filename) && Utils::file_hash(m_original_request->hash_algorithm, m_filename) == m_original_request->filehash) {
            // file is already up to date, don't need to download it again
            throw Errors::Transfer::UpToDateError(m_filename);
        }

        std::filesystem::create_directories(std::filesystem::path(m_temp_filename).parent_path());
        m_file.open(m_temp_filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
    }

    void DownloadTransferHandler::receive_packet(const Protocol::DataPacketData &data) {
        auto missing = std::ranges::find(m_missing_ids, data.packet_id);

        if (missing != m_missing_ids.end())
            m_missing_ids.erase(missing);

        m_transferred_size += data.data.size();
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
            if (m_missing_ids.empty() && (m_expected_id + 1) == m_original_request->total_packets) {
                finish_transfer();
            }
        } else { // data.packet_id == m_expected_id
            bool last_packet;

            // TODO FIXME: this breaks with files of size 0
            m_expected_id++; // Increment before comparaison, cause if we need 2 total packets, we will receive ids 0 and 1.
            last_packet = m_expected_id == m_original_request->total_packets;

            m_file.write(data.data.data(), data.data.size());
            if (data.data.size() != m_original_request->packet_size && !last_packet) {
                // TODO: something is wrong if this happens -> figure out what to do.
                throw std::runtime_error("Transfert size invalid");
            }
            if (last_packet && m_missing_ids.empty()) {
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

    auto DownloadTransferHandler::finished() const -> bool {
        return !m_file.is_open();
    }

    auto IFileTransferHandler::get_current_size() const -> std::size_t {
        // TODO: this is not exact : last packet might have a smaller size
        // return m_expected_id * m_original_request->packet_size;
        return m_transferred_size;
    }

    auto IFileTransferHandler::get_total_size() const -> std::size_t {
        // TODO: this is not exact : last packet might have a smaller size
        return m_original_request->total_packets * m_original_request->packet_size;
    }

    auto IFileTransferHandler::get_original_request() const -> std::shared_ptr<Protocol::SendFileData> {
        return m_original_request;
    }

    UploadTransferHandler::UploadTransferHandler(const std::string &filepath, std::shared_ptr<Protocol::SendFileData> original_request, std::size_t packet_start) {
        m_original_request = std::move(original_request);
        m_file.open(filepath);
        // TODO: this won't raise on fail to open / fail to seek (need failibt, but failbit would raise if EOF while reading...)
        m_file.exceptions(std::ifstream::badbit); // Enable exceptions on IO operations
        m_file.seekg(m_original_request->packet_size * packet_start);
    }

    // TODO: Do we really need shared_ptrs for the transfer packets ?
    auto UploadTransferHandler::get_next_packet(Protocol::MessageID original_request_id) -> std::shared_ptr<Protocol::DataPacketData> {
        std::vector<char> buffer;
        std::string data;
        std::shared_ptr<Protocol::DataPacketData> data_packet_data;

        if (finished())
            return nullptr;

        buffer.reserve(m_original_request->packet_size);
        m_file.read(buffer.data(), m_original_request->packet_size);
        data = std::string(buffer.data(), m_file.gcount());
        data_packet_data = std::make_shared<Protocol::DataPacketData>(original_request_id, m_packet_id++, data);

        m_transferred_size += data.size();
        if (data.size() < m_original_request->packet_size) {
            m_file.close();
        }
        return data_packet_data;
    }

    auto UploadTransferHandler::finished() const -> bool {
        return !m_file.is_open();
    }

    ListFilesTransferHandler::ListFilesTransferHandler(std::filesystem::path requested_path, FileMapping &file_mapping, std::size_t packet_size) :
        m_requested_path(std::move(requested_path)), m_file_mapping(file_mapping), m_packet_size(packet_size)
    {
        std::filesystem::path::iterator out;

        if (m_requested_path.empty())
            m_requested_path = std::filesystem::path("//") / m_file_mapping.get_root_name();
        m_path_node = file_mapping.find_virtual_node(m_requested_path, out);
        if (!m_path_node.has_value()) {
            return;
        }
        if (m_path_node->is_host_folder()) {
            std::filesystem::path host_path = FileShare::FileMapping::virtual_to_host(m_requested_path, m_path_node, out);

            if (!host_path.empty()) {
                m_directory_iterator = std::filesystem::directory_iterator(host_path);
            }
        } else if (m_path_node->is_virtual()) {
            m_node_iterator = m_path_node->get_child_nodes().begin();
        }
    }

    auto ListFilesTransferHandler::get_next_packet(Protocol::MessageID original_request_id) -> std::shared_ptr<Protocol::FileListData> {
        if (m_extra_packet_sent || !m_path_node.has_value()) {
            return nullptr;
        }

        std::vector<Protocol::FileInfo> vector;
        const std::size_t max_count = 255; // TODO: determine that dynamically from packet_size

        switch (m_path_node->get_type()) {
            case PathNode::HOST_FOLDER: {
                const auto end = std::filesystem::directory_iterator();

                for (std::size_t i = 0; m_directory_iterator != end && i < max_count; i++) {
                    auto filepath = m_requested_path / m_directory_iterator->path().filename();
                    auto file_type = m_directory_iterator->is_directory() ? Protocol::FileType::DIRECTORY : Protocol::FileType::FILE;

                    vector.emplace_back(Protocol::FileInfo{.path=filepath.string(), .file_type=file_type});
                    m_directory_iterator++;
                }
                break;
            }
            case PathNode::HOST_FILE: {
                if (m_current_id != 0) {
                    break; // Send this only once
                }
                std::filesystem::directory_entry entry(m_path_node->get_host_path());

                if (entry.is_directory()) {
                    break; // It's supposed to be a file, abort
                }
                vector.emplace_back(Protocol::FileInfo{.path=m_requested_path.string(), .file_type=Protocol::FileType::FILE});
                break;
            }
            case PathNode::VIRTUAL: {
                const auto &nodes = m_path_node->get_child_nodes();

                for (std::size_t i = 0; i < max_count && m_node_iterator != nodes.end(); i++) {
                    const PathNode &node = m_node_iterator->second;
                    auto file_type = node.is_host_file() ? Protocol::FileType::FILE : Protocol::FileType::DIRECTORY;

                    vector.emplace_back(Protocol::FileInfo{.path=(m_requested_path / node.get_name()).string(), .file_type=file_type});
                    m_node_iterator++;
                }
                break;
            }
        }
        auto request = std::make_shared<Protocol::FileListData>(original_request_id, m_current_id++, std::move(vector));

        // TODO: Can we get rid of the extra empty packet ?
        if (request->files.empty()) {
            m_extra_packet_sent = true;
        }
        return request;
    }

    auto ListFilesTransferHandler::finished() const -> bool {
        if (!m_path_node.has_value())
            return true;
        switch (m_path_node->get_type()) {
            case PathNode::HOST_FOLDER:
                return m_directory_iterator == std::filesystem::directory_iterator();
            case PathNode::HOST_FILE:
                return m_current_id == 1;
            case PathNode::VIRTUAL:
                return m_node_iterator == m_path_node->get_child_nodes().end();
        }
    }

    void FileListTransferHandler::receive_packet(Protocol::FileListData data) {
        if (data.packet_id > m_current_id) {
            while (m_current_id < data.packet_id) {
                m_missing_ids.push_back(m_current_id++);
            }
        } else if (data.packet_id < m_current_id) {
            auto iter = std::ranges::find(m_missing_ids, data.packet_id);

            if (iter != m_missing_ids.end()) {
                m_missing_ids.erase(iter);
            } else {
                return; // Don't receive twice the same packet
            }
        } else {
            m_current_id++;
        }
        if (data.files.empty()) {
            m_finished = true;
        } else {
            m_file_list.insert(m_file_list.end(), std::make_move_iterator(data.files.begin()), std::make_move_iterator(data.files.end()));
        }
    }

    auto FileListTransferHandler::finished() const -> bool {
        return m_missing_ids.empty() && m_finished;
    }
}
