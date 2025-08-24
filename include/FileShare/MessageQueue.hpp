/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue May  9 09:33:48 2023 Francois Michaut
** Last update Fri Aug 15 13:46:20 2025 Francois Michaut
**
** MessageQueue.hpp : A queue representing the messages sent/received and their status
*/

#pragma once

#include "FileShare/Protocol/Definitions.hpp"

#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace FileShare {
    struct Message {
        Protocol::Request request;
        std::optional<Protocol::StatusCode> status;
        std::string raw_message; // TODO: use it
    };

    class MessageQueue {
        public:
            using MessageMap = std::unordered_map<Protocol::MessageID, Message>;

            static constexpr Protocol::MessageID MAX_ID = 0xFF;

            MessageQueue() = default;

            auto available_send_slots() const -> std::uint8_t { return m_available_send_slots; }

            auto send_request(Protocol::Request request) -> Protocol::MessageID; // append to m_outgoing_requests
            auto receive_request(Protocol::Request request) -> Protocol::MessageID; // append to m_incomming_requests
            void send_reply(Protocol::MessageID request_id, Protocol::StatusCode status_code);
            void receive_reply(Protocol::MessageID request_id, Protocol::StatusCode status_code);

            auto get_outgoing_requests() const -> const MessageMap & { return m_outgoing_requests; }
            auto get_incomming_requests() const -> const MessageMap & { return m_incomming_requests; }

        private:
            Protocol::MessageID m_message_id = 0;
            MessageMap m_outgoing_requests;
            MessageMap m_incomming_requests;
            std::uint8_t m_available_send_slots = MAX_ID;
    };
}
