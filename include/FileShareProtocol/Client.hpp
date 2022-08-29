/*
** Project LibFileShareProtocol, 2022
**
** Author Francois Michaut
**
** Started on  Sun Aug 28 09:23:07 2022 Francois Michaut
** Last update Sun Aug 28 19:16:16 2022 Francois Michaut
**
** Client.hpp : Client to communicate with peers with the FileShareProtocol
*/

#include "FileShareProtocol/Definitions.hpp"

#include <string>

namespace FileShareProtocol {
    class Client {
        public:
            Client(); // TODO: Client constructor require an endpoint/socket

            StatusCode send_file(std::string filepath);
            StatusCode receive_file(std::string filepath);
            StatusCode list_files(std::size_t page_idx, std::string folderpath = "");

            // TODO determine params
            StatusCode initiate_pairing();
            StatusCode accept_pairing();
        private:
    };
}
