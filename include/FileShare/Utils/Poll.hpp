/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Fri Jul 25 18:19:49 2025 Francois Michaut
** Last update Mon Aug 18 16:26:06 2025 Francois Michaut
**
** Poll.hpp : Cross-Plateform poll implementation
*/

#pragma once

#include <vector>

#include <CppSockets/OSDetection.hpp>

#ifdef OS_UNIX
    #include <poll.h>
#else // Windows only
    using nfds_t=std::size_t;
#endif

namespace FileShare::Utils {
    // TODO: Add support for Signals
    auto poll(std::vector<struct pollfd> &fds, const struct timespec *timeout = nullptr) -> int;
    auto poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout = nullptr) -> int;
}
