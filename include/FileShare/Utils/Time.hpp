/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue Jul 18 08:17:11 2023 Francois Michaut
** Last update Tue Nov 28 13:30:18 2023 Francois Michaut
**
** Time.hpp : Time utilities
*/

#pragma once

#include <chrono>

#include <CppSockets/OSDetection.hpp>

namespace FileShare::Utils {
    // TODO: see if this is portable / reliable
    template<class Clock>
    std::uint64_t to_epoch(std::chrono::time_point<Clock> time) {
#ifdef OS_APPLE
        auto system_time = std::chrono::file_clock::to_sys(time);
#else
        auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(time);
#endif
        std::uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(system_time.time_since_epoch()).count();

        return epoch;
    }

    template<class Clock>
    std::chrono::time_point<Clock> from_epoch(std::uint64_t epoch) {
        auto duration = std::chrono::seconds(epoch);
        std::chrono::time_point<Clock> time_point(duration);

        return time_point;
    }
}
