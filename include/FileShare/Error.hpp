/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Oct 22 13:22:53 2023 Francois Michaut
** Last update Sun Oct 22 13:27:05 2023 Francois Michaut
**
** Error.hpp : Generic Error class
*/

#pragma once

#include <stdexcept>

namespace FileShare {
    class Error : public std::runtime_error {
        public:
            Error(const char *message) : std::runtime_error(message) {}
            Error(const std::string &message) : std::runtime_error(message) {}
    };
}
