/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Thu Aug 25 22:59:37 2022 Francois Michaut
** Last update Tue May  9 08:51:20 2023 Francois Michaut
**
** Protocol.hpp : Main class to interract with the protocol
*/

#pragma once

#include "FileShare/Protocol/Definitions.hpp"
#include "FileShare/Protocol/Version.hpp"

#include <map>
#include <memory>

namespace FileShare::Protocol {
    class IProtocolHandler {
        public:
            static constexpr char const * const magic_bytes = "FSP_";

            virtual std::string format_send_file(std::string filepath) = 0;
            virtual std::string format_receive_file(std::string filepath) = 0;
            virtual std::string format_list_files(std::string folderpath = "", std::size_t page_idx = 0) = 0;
    };

    class Protocol {
        public:
            Protocol(Version version);

            void set_version(Version version);
            Version version() const { return m_version; }

            bool operator==(const Protocol &) const = default;
            auto operator<=>(const Protocol&) const = default;

            IProtocolHandler &handler() const { return *m_handler; }

            Response<void> send_file(std::string filepath);
            Response<void> receive_file(std::string filepath);
            Response<FileList> list_files(std::string folderpath = "", std::size_t page_idx = 0);

            static std::map<Version, std::shared_ptr<IProtocolHandler>> protocol_list;
        private:
            Version m_version;
            std::shared_ptr<IProtocolHandler> m_handler;
    };
}
