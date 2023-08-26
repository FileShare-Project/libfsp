/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue May  9 09:33:48 2023 Francois Michaut
** Last update Thu Aug 24 19:34:07 2023 Francois Michaut
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
            using MessageMap = std::unordered_map<std::uint8_t, Message>;

            MessageQueue() = default;

            std::uint8_t available_send_slots() const;

            std::uint8_t send_request(Protocol::Request request); // append to m_outgoing_requests
            std::uint8_t receive_request(Protocol::Request request); // append to m_incomming_requests
            void send_reply(std::uint8_t request_id, Protocol::StatusCode status_code);
            void receive_reply(std::uint8_t request_id, Protocol::StatusCode status_code);

            const MessageMap &get_outgoing_requests() const;
            const MessageMap &get_incomming_requests() const;
        private:
            std::uint8_t m_message_id = 0;
            MessageMap m_outgoing_requests;
            MessageMap m_incomming_requests;
            std::uint8_t m_available_send_slots = 0xFF;
    };
}
