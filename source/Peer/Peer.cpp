/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Mon Aug 29 20:50:53 2022 Francois Michaut
** Last update Sat Aug 23 12:59:50 2025 Francois Michaut
**
** Peer.cpp : Implementation of the FileShareProtocol Client
*/

#include "FileShare/Peer/Peer.hpp"
#include "FileShare/Peer/PeerBase.hpp"
#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Protocol/RequestData.hpp"
#include "FileShare/Server.hpp"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

#include <utility>

// TODO: Use custom classes for exceptions
namespace FileShare {
    Peer::Peer(PreAuthPeer &&peer, FileShare::Config config) :
        PeerBase(std::move(peer)), // TODO: Check its correct to move into base, and still use it afterwards
        m_config(std::move(config)), m_protocol(peer.get_protocol())
    {}

    auto Peer::pull_requests() -> std::vector<Protocol::Request> {
        std::vector<Protocol::Request> result;

        poll_requests();
        // Move the buffer in the result, and clears the buffer
        // Requests will be lost if callers discards them. TODO: improve ? Could clear when user call `respond_to_request()`
        m_request_buffer.swap(result);
        return result;
    }

    void Peer::respond_to_request(Protocol::Request request, Protocol::StatusCode status) {
        m_message_queue.receive_request(request);

        if (status != Protocol::StatusCode::STATUS_OK) {
            send_reply(request.message_id, status);
            return;
        }

        switch (request.code) {
            case Protocol::CommandCode::DATA_PACKET: {
                auto data = std::dynamic_pointer_cast<Protocol::DataPacketData>(request.request);
                auto iter = m_download_transfers.find(data->request_id);

                if (iter != m_download_transfers.end()) {
                    auto &handler = iter->second;

                    handler.receive_packet(*data);
                    if (handler.finished() && !handler.m_keep) {
                        m_download_transfers.erase(data->request_id);
                    }
                    break;
                }
                send_reply(request.message_id, Protocol::StatusCode::INVALID_REQUEST_ID);
                return;
            }
            case Protocol::CommandCode::SEND_FILE: {
                std::shared_ptr<Protocol::SendFileData> data = std::dynamic_pointer_cast<Protocol::SendFileData>(request.request);

                create_download(request.message_id, data);
                return; // create_download already sends reply to request

            }
            case Protocol::CommandCode::RECEIVE_FILE: {
                std::shared_ptr<Protocol::ReceiveFileData> data = std::dynamic_pointer_cast<Protocol::ReceiveFileData>(request.request);
                auto virtual_path = data->filepath;
                auto host_path = m_config.get_file_mapping().virtual_to_host(virtual_path);
                auto [handler, status] = prepare_upload(host_path.string(), virtual_path, data->packet_size, data->packet_start);

                // Send reply to original RECEIVE_FILE before we send SEND_FILE
                send_reply(request.message_id, status);
                if (status == Protocol::StatusCode::STATUS_OK) {
                    // prepare_upload will always return a handler if status == OK
                    create_upload(std::move(handler.value())); // NOLINT(bugprone-unchecked-optional-access)
                }
                return;
            }
            case Protocol::CommandCode::LIST_FILES: {
                auto data = std::dynamic_pointer_cast<Protocol::ListFilesData>(request.request);
                constexpr std::size_t packet_size = 4096; // TODO: find a better place for this, duplicated from ProtocolHandler
                ListFilesTransferHandler handler(data->folderpath, m_config.get_file_mapping(), packet_size);

                auto result = m_list_files_transfers.emplace(std::piecewise_construct, std::forward_as_tuple(request.message_id), std::forward_as_tuple(std::move(handler)));

                send_reply(request.message_id, Protocol::StatusCode::STATUS_OK);
                // TODO: Send more than one
                send_request(Protocol::CommandCode::FILE_LIST, result.first->second.get_next_packet(request.message_id));
                return;
            }
            case Protocol::CommandCode::FILE_LIST: {
                auto data = std::dynamic_pointer_cast<Protocol::FileListData>(request.request);
                auto handler = m_file_list_transfers.find(data->request_id);

                if (handler != m_file_list_transfers.end()) {
                    handler->second.receive_packet(*data);
                    break;
                }
                send_reply(request.message_id, Protocol::StatusCode::INVALID_REQUEST_ID);
                return;
            }
            default:
                break;
        }
        send_reply(request.message_id, Protocol::StatusCode::STATUS_OK);
    }

    auto Peer::default_config() -> Config {
        return Server::default_peer_config();
    }

    auto Peer::send_file(const std::string &filepath, const ProgressCallback &progress_callback) -> Protocol::Response<void> {
        auto result = create_host_upload(filepath);
        Protocol::MessageID message_id = result->first;
        UploadTransferHandler &upload_handler = result->second;
        Protocol::StatusCode status = wait_for_status(message_id);

        // TODO: handle APPROVAL_PENDING
        if (status != Protocol::StatusCode::STATUS_OK) {
            m_upload_transfers.erase(result);
            return {.code=status, .response={}};
        }
        while (!upload_handler.finished()) {
            if (m_message_queue.available_send_slots() > 0) {
                auto packet = upload_handler.get_next_packet(message_id);

                // TODO: monitor responses of theses packets
                send_request(Protocol::CommandCode::DATA_PACKET, packet);
                progress_callback(filepath, upload_handler.get_current_size(), upload_handler.get_total_size());
            } else {
                // TODO: currently blocking, but if it changes, needs to add a poll() call to avoid spamming loop
                poll_requests();
            }
        }

        return {.code=status, .response={}}; // TODO
    }

    auto Peer::receive_file(std::string filepath, const ProgressCallback &progress_callback) -> Protocol::Response<void> {
        std::size_t packet_start = 0; // TODO
        std::size_t packet_size = 0; // TODO
        std::shared_ptr<Protocol::ReceiveFileData> receive_file_data = std::make_shared<Protocol::ReceiveFileData>(filepath, packet_size, packet_start);
        Protocol::MessageID message_id = send_request(Protocol::CommandCode::RECEIVE_FILE, receive_file_data);
        Protocol::StatusCode status = wait_for_status(message_id);

        // TODO: handle APPROVAL_PENDING
        if (status != Protocol::StatusCode::STATUS_OK) {
            return {.code=status, .response={}};
        }

        auto incomming_requests = m_message_queue.get_incomming_requests();
        auto request = std::ranges::find_if(incomming_requests, [&filepath](const auto &item) {
            if (item.second.request.code != Protocol::CommandCode::SEND_FILE) {
                return false;
            }

            auto original_data = std::dynamic_pointer_cast<Protocol::SendFileData>(item.second.request.request);
            return filepath == original_data->filepath;
        });
        auto transfer_iter = m_download_transfers.find(request->first);
        if (request != incomming_requests.end() && request->second.status.has_value()
            && request->second.status.value() == Protocol::StatusCode::UP_TO_DATE // NOLINT(bugprone-unchecked-optional-access)
        ) {
            return {.code = Protocol::StatusCode::UP_TO_DATE, .response = {}};
        }
        if (request == incomming_requests.end() || transfer_iter == m_download_transfers.end()) {
            throw std::runtime_error("Failed to locate download transfer"); // TODO: we got STATUS_OK but no incomming request ? Something is wrong
        }
        auto &transfer_handler = transfer_iter->second;

        transfer_handler.m_keep = true;
        while (!transfer_handler.finished()) {
            progress_callback(filepath, transfer_handler.get_current_size(), transfer_handler.get_total_size());
            poll_requests(); // TODO: currently blocking, but if it changes, needs to add a poll() call to avoid spamming loop
        }

        m_download_transfers.erase(transfer_iter);
        return {.code=status, .response={}}; // TODO
    }

    auto Peer::list_files(std::string folderpath) -> Protocol::Response<std::vector<Protocol::FileInfo>> {
        std::shared_ptr<Protocol::ListFilesData> list_files_data = std::make_shared<Protocol::ListFilesData>(folderpath);
        Protocol::MessageID message_id = send_request(Protocol::CommandCode::LIST_FILES, list_files_data);
        Protocol::StatusCode status = wait_for_status(message_id);

        if (status != Protocol::StatusCode::STATUS_OK) {
            return {.code=status, .response={}};
        }
        auto iter = m_file_list_transfers.find(message_id);

        if (iter == m_file_list_transfers.end()) {
            throw std::runtime_error("Failed to locate list file transfer"); // TODO: we got STATUS_OK but no transfer? something is wrong.
        }
        auto &handler = iter->second;

        while (!handler.finished()) {
            poll_requests(); // TODO: currently blocking, but if it changes, needs to add a poll() call to avoid spamming loop
        }

        auto file_list = std::make_shared<std::vector<Protocol::FileInfo>>(handler.get_file_list());

        // TODO: We need to erase from file_list_transfers for async somehow as well
        m_file_list_transfers.erase(message_id);
        return {.code=status, .response=file_list};
    }
}

// TODO: there are many places where client will loop forever if socket is not connected, fix it.
