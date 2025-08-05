/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Fri Jul 25 18:19:49 2025 Francois Michaut
** Last update Sat Aug 23 20:43:12 2025 Francois Michaut
**
** Poll.hpp : Cross-Plateform poll implementation
*/

#include "FileShare/Utils/Poll.hpp"

#ifdef OS_WINDOWS
    static auto &poll=WSAPoll; // alias function poll to WSAPoll
#endif

namespace FileShare::Utils {
    auto poll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout) -> int {
        int nb_ready = 0;

#ifdef OS_LINUX
        nb_ready = ppoll(fds, nfds, timeout, nullptr);
#else
        int timeout_ms = 0;

        if (timeout == nullptr) {
            timeout_ms = -1; // Infinite Timeout
        } else {
            timeout_ms = (timeout->tv_sec * 1000) + (timeout->tv_nsec / 1000000);
        }
        nb_ready = poll(fds, nfds, timeout_ms);
#endif

        // TODO: Error management ?
        // TODO: Signal Handling ?
        return nb_ready;
    }

    auto poll(std::vector<struct pollfd> &fds, const struct timespec *timeout) -> int {
        return poll(fds.data(), fds.size(), timeout);
    }
}
