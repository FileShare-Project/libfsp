/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue Jul 18 08:17:11 2023 Francois Michaut
** Last update Thu Aug 14 19:27:50 2025 Francois Michaut
**
** Time.hpp : Time utilities
*/

#pragma once

#include <CppSockets/OSDetection.hpp>

#include <chrono>

namespace FileShare::Utils {
    // TODO: see if this is portable / reliable
    template<class Clock>
    auto to_epoch(std::chrono::time_point<Clock> time) -> std::uint64_t {
#ifdef OS_APPLE
        auto system_time = std::chrono::file_clock::to_sys(time);
#else
        auto system_time = std::chrono::clock_cast<std::chrono::system_clock>(time);
#endif
        std::uint64_t epoch = std::chrono::duration_cast<std::chrono::seconds>(system_time.time_since_epoch()).count();

        return epoch;
    }

    template<class Clock>
    auto from_epoch(std::uint64_t epoch) -> std::chrono::time_point<Clock> {
        auto duration = std::chrono::seconds(epoch);
        std::chrono::time_point<Clock> time_point(duration);

        return time_point;
    }
}
