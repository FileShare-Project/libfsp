/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue Aug 22 18:25:07 2023 Francois Michaut
** Last update Sat Aug 26 17:37:43 2023 Francois Michaut
**
** MessageQueue.cpp : Implementation of the queue representing the messages sent/received and their status
*/

#include "FileShare/MessageQueue.hpp"

namespace FileShare {
    std::uint8_t MessageQueue::send_request(Protocol::Request request) {
        if (m_available_send_slots == 0) {
            throw std::runtime_error("No more available slots");
        }
        m_available_send_slots--;

        while (m_outgoing_requests.contains(m_message_id)) {
            if (m_outgoing_requests.at(m_message_id).status.has_value())
                break; // If there is a status already, this is an old request, we can replace it
                       // TODO: also ignore APPROVAL_PENDING
            if (m_message_id == 255) {
                m_message_id = 0;
            } else {
                m_message_id++;
            }
        }
        request.message_id = m_message_id;
        m_outgoing_requests[m_message_id] = Message{std::move(request), {}, ""};
        return m_message_id;
    }

    std::uint8_t MessageQueue::receive_request(Protocol::Request request) {
        m_incomming_requests[request.message_id] = Message{std::move(request), {}, ""};
        return request.message_id;
    }

    void MessageQueue::send_reply(std::uint8_t request_id, Protocol::StatusCode status_code) {
        m_incomming_requests.at(request_id).status = status_code;
    }

    void MessageQueue::receive_reply(std::uint8_t request_id, Protocol::StatusCode status_code) {
        m_outgoing_requests.at(request_id).status = status_code;
        m_available_send_slots++;
    }

    std::uint8_t MessageQueue::available_send_slots() const {
        return m_available_send_slots;
    }

    const MessageQueue::MessageMap &MessageQueue::get_outgoing_requests() const {
        return m_outgoing_requests;
    }

    const MessageQueue::MessageMap &MessageQueue::get_incomming_requests() const {
        return m_incomming_requests;
    }
}
