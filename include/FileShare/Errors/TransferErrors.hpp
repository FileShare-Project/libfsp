/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Oct 22 13:22:09 2023 Francois Michaut
** Last update Sun Oct 22 13:54:04 2023 Francois Michaut
**
** TransferErrors.hpp : Transfer related errors
*/

#pragma once

#include "FileShare/Error.hpp"

namespace FileShare::Errors::Transfer {
    class UpToDateError : public FileShare::Error {
        public:
            UpToDateError(const char *filename);
            UpToDateError(std::string filename);

        private:
            std::string m_filename;
    };
}
