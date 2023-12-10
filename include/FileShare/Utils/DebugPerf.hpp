/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue May  9 10:08:36 2023 Francois Michaut
** Last update Sun Dec 10 18:43:47 2023 Francois Michaut
**
** DebugPerf.hpp : A helper class to display time spent in functions or other scopes
*/

#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace FileShare::Utils {
#ifdef DEBUG
    class DebugPerf {
        public:
            DebugPerf(std::string name);
            DebugPerf(const DebugPerf &other) = default;
            ~DebugPerf();

        private:
            static thread_local DebugPerf *m_current;

            void output_perf_results(std::size_t level = 0) const;

            std::chrono::time_point<std::chrono::steady_clock> m_start;
            std::chrono::time_point<std::chrono::steady_clock> m_end;
            std::string m_name;
            DebugPerf *m_parent;
            std::vector<DebugPerf> m_childs;
    };
#else
    class DebugPerf {
        public:
            DebugPerf(std::string) {}
    };
#endif
}
