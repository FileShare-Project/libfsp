/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:23:07 2022 Francois Michaut
** Last update Mon Aug 29 21:10:44 2022 Francois Michaut
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
            Client(const CppSockets::IEndpoint &server_peer);
            Client(CppSockets::Socket &&server_peer);

            StatusCode send_file(std::string filepath);
            StatusCode receive_file(std::string filepath);
            StatusCode list_files(std::size_t page_idx = 0, std::string folderpath = "");

            // TODO determine params
            StatusCode initiate_pairing();
            StatusCode accept_pairing();
        private:
            CppSockets::Socket socket;
    };
}
