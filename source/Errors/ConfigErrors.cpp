/*
** Project LibFileShareProtocol, 2026
**
** Author Francois Michaut
**
** Started on  Thu May 14 04:40:23 2026 Francois Michaut
** Last update Thu May 14 04:41:08 2026 Francois Michaut
**
** ConfigErrors.cpp : Config related errors implementation
*/

#include "FileShare/Errors/ConfigErrors.hpp"

namespace FileShare::Errors::Config {
    NotFoundError::NotFoundError(const char *path) :
        NotFoundError(std::string(path))
    {}

    NotFoundError::NotFoundError(std::string path) :
        Error("Config file '" + path + "' not found"), m_path(std::move(path))
    {}
}
