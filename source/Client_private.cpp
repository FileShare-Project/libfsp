/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Mon Oct 23 21:33:10 2023 Francois Michaut
** Last update Wed Oct 25 21:50:27 2023 Francois Michaut
**
** Client_private.cpp : Private functions of Client implementation
*/

#include "FileShare/Client.hpp"

#ifdef OS_UNIX
    #include <poll.h>
#endif

namespace FileShare {
    void Client::poll_requests() {
        std::size_t total = 0;
        std::string_view view;
        Protocol::Request request;
        std::size_t ret = 0;

        if (!m_socket.connected())
            return;
        m_buffer += m_socket.read();
        if (m_buffer.empty()) {
            return;
        }
        view = m_buffer;
        while (true) {
            ret = m_protocol.handler().parse_request(view, request);
            if (ret == 0) {
                break;
            }
            authorize_request(request);
            total += ret;
            view = view.substr(ret);
        }
        m_buffer = m_buffer.substr(total);
    }

    void Client::authorize_request(Protocol::Request request) {
        // TODO: implement permission logic with allowed / denied files in the Config
        // TODO: implement retries logic
        if (request.code == Protocol::CommandCode::RESPONSE) {
            auto data = std::dynamic_pointer_cast<Protocol::ResponseData>(request.request);
            auto outgoing_requests = m_message_queue.get_outgoing_requests();

            if (outgoing_requests.contains(request.message_id)) {
                auto source_request = outgoing_requests.at(request.message_id).request;

                receive_reply(request.message_id, data->status);
                if (source_request.code == Protocol::CommandCode::DATA_PACKET) {
                    auto packet_data = std::dynamic_pointer_cast<Protocol::DataPacketData>(source_request.request);
                    auto &handler = m_upload_transfers.at(packet_data->request_id);
                    std::shared_ptr<Protocol::DataPacketData> new_packet = handler.get_next_packet(packet_data->request_id);

                    if (new_packet) {
                        send_request(Protocol::CommandCode::DATA_PACKET, new_packet);
                    }
                }
            }
        } else if (request.code == Protocol::CommandCode::DATA_PACKET) {
            auto data = std::dynamic_pointer_cast<Protocol::DataPacketData>(request.request);

            if (m_download_transfers.contains(data->request_id)) {
                respond_to_request(request, Protocol::StatusCode::STATUS_OK);
            } else {
                respond_to_request(request, Protocol::StatusCode::INVALID_REQUEST_ID);
            }
        } else {
            // The request has not be auto-approved or rejected.
            // It will go through manual approval
            // TODO: send status APPROVAL_PENDING ?
            m_request_buffer.emplace_back(std::move(request));
        }
    }

    void Client::receive_reply(std::uint8_t message_id, Protocol::StatusCode status) {
        m_message_queue.receive_reply(message_id, status);
    }

    void Client::send_reply(std::uint8_t message_id, Protocol::StatusCode status) {
        auto response_data = std::make_shared<Protocol::ResponseData>(status);
        std::string message = m_protocol.handler().format_response(message_id, status);

        m_message_queue.send_reply(message_id, status);
        m_socket.write(message);
    }

    std::uint8_t Client::send_request(Protocol::CommandCode command, std::shared_ptr<Protocol::IRequestData> request_data) {
        return send_request({command, request_data});
    }

    std::uint8_t Client::send_request(Protocol::Request request) {
        std::uint8_t message_id = m_message_queue.send_request(request);
        std::string message;

        request.message_id = message_id;
        message = m_protocol.handler().format_request(request);
        m_socket.write(message);
        return message_id;
    }

    // TODO: deprecate
    Protocol::StatusCode Client::wait_for_status(std::uint8_t message_id) {
        auto &message = m_message_queue.get_outgoing_requests().at(message_id);

        // TODO HACK: this is not good as we could wait forever if peer is
        // trying to DDOS us by not sending a response ever.
        // Implement async model instead
        while (!message.status.has_value()) {
            std::array<struct pollfd, 1> fds;
            nfds_t nfds = 1;
            int nb_ready = 0;

            fds[0] = {m_socket.get_fd(), POLLIN, 0};
            nb_ready = ppoll(fds.data(), nfds, nullptr, nullptr); // TODO: add timeout
            if (nb_ready < 0) // TODO: handle signals
                throw std::runtime_error("Failed to poll for status");
            poll_requests();
            if (!m_socket.connected())
                throw std::runtime_error("connection lost while waiting for status");
        }
        return message.status.value();
    }
}
