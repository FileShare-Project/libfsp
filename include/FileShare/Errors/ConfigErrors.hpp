/*
** Project LibFileShareProtocol, 2026
**
** Author Francois Michaut
**
** Started on  Thu May 14 04:34:25 2026 Francois Michaut
** Last update Sat May 16 11:16:25 2026 Francois Michaut
**
** ConfigErrors.hpp : Config related errors
*/

#pragma once

#include "FileShare/Error.hpp"

#include <string>

namespace FileShare::Errors::Config {
    class NotFoundError : public FileShare::Error {
        public:
            NotFoundError(const char *path);
            NotFoundError(std::string path);

        private:
            std::string m_path;
    };
}
