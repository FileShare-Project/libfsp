/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Oct 22 13:51:24 2023 Francois Michaut
** Last update Sat Dec  9 08:57:23 2023 Francois Michaut
**
** TransferErrors.cpp : Transfer related errors implementation
*/

#include "FileShare/Errors/TransferErrors.hpp"

namespace FileShare::Errors::Transfer {
    UpToDateError::UpToDateError(const char *filename) :
        UpToDateError(std::string(filename))
    {}

    UpToDateError::UpToDateError(std::string filename) :
        Error("File '" + filename + "' is already up to date"), m_filename(std::move(filename))
    {}
}
