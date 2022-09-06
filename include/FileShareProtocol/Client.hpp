/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:23:07 2022 Francois Michaut
** Last update Tue Sep  6 14:04:54 2022 Francois Michaut
**
** Client.hpp : Client to communicate with peers with the FileShareProtocol
*/

#include "FileShareProtocol/Definitions.hpp"

#include <CppSockets/IPv4.hpp>
#include <CppSockets/Socket.hpp>
#include <CppSockets/Version.hpp>

#include <string>

namespace FileShareProtocol {
    class Client {
        public:
            Client(const CppSockets::IEndpoint &peer);
            Client(CppSockets::Socket &&peer);

            void reconnect(const CppSockets::IEndpoint &peer);
            void reconnect(CppSockets::Socket &&peer);

            Response<void> send_file(std::string filepath);
            Response<void> receive_file(std::string filepath);
            Response<void> list_files(std::size_t page_idx = 0, std::string folderpath = "");

            // TODO determine params
            Response<void> initiate_pairing();
            Response<void> accept_pairing();
        private:
            CppSockets::Socket socket;
    };
}
