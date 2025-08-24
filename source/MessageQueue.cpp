/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue Aug 22 18:25:07 2023 Francois Michaut
** Last update Fri Aug 15 14:36:03 2025 Francois Michaut
**
** MessageQueue.cpp : Implementation of the queue representing the messages sent/received and their status
*/

#include "FileShare/MessageQueue.hpp"

#include <stdexcept>

namespace FileShare {
    auto MessageQueue::send_request(Protocol::Request request) -> Protocol::MessageID {
        if (m_available_send_slots == 0) {
            throw std::runtime_error("No more available slots");
        }
        m_available_send_slots--;

        while (m_outgoing_requests.contains(m_message_id)) {
            auto status = m_outgoing_requests.at(m_message_id).status;

            // If there is a status already, this is an old request, we can replace it
            // Except for APPROVAL_PENDING, since we are waiting for an anwser
            if (status.has_value() && status.value() != Protocol::StatusCode::APPROVAL_PENDING) {
                // NOTE: Not vulnerable to DOS attack, since peer would only be able to DOS
                // its own connection if it fills our message_queue with APPROVAL_PENDING
                break;
            }
            if (m_message_id == MAX_ID) {
                m_message_id = 0;
            } else {
                m_message_id++;
            }
        }
        request.message_id = m_message_id;
        m_outgoing_requests[m_message_id] = Message{std::move(request), {}, ""};
        return m_message_id++; // Will return value before incrementation
    }

    auto MessageQueue::receive_request(Protocol::Request request) -> Protocol::MessageID {
        Protocol::MessageID message_id = request.message_id;

        // TODO: Peer could override its own requests, make sure that doesn't break things
        m_incomming_requests[message_id] = Message{std::move(request), {}, ""};
        return message_id;
    }

    void MessageQueue::send_reply(Protocol::MessageID request_id, Protocol::StatusCode status_code) {
        m_incomming_requests.at(request_id).status = status_code;
    }

    void MessageQueue::receive_reply(Protocol::MessageID request_id, Protocol::StatusCode status_code) {
        // TODO: Will throw if peer sends an invalid reply
        auto &request = m_outgoing_requests.at(request_id);
        bool is_approval_pending = false;
        bool has_value = request.status.has_value();

        if (has_value) {
            auto value = request.status.value();

            if (value == status_code) {
                return;
            }
            is_approval_pending = value == Protocol::StatusCode::APPROVAL_PENDING;
        }
        // We only increment slots if there is no value, or the value is APPROVAL_PENDING,
        // only if new value is not APPROVAL_PENDING.
        // This is because if there is already a value; we don't want to count it twice,
        // except for APPROVAL_PENDING since it is the only value we don't count
        if ((!has_value || is_approval_pending) && status_code != Protocol::StatusCode::APPROVAL_PENDING) {
            m_available_send_slots++;
        }
        request.status = status_code;
    }
}
