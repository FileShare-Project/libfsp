/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Tue May  9 10:13:38 2023 Francois Michaut
** Last update Tue Jul 18 22:02:38 2023 Francois Michaut
**
** DebugPerf.cpp : Implementation of helper class to display time spent in functions or other scopes
*/

#include "FileShare/Utils/DebugPerf.hpp"

#ifdef DEBUG
#include <iostream>

namespace FileShare::Utils {
    thread_local DebugPerf *DebugPerf::m_current = nullptr;

    DebugPerf::DebugPerf(std::string name) :
        m_name(name)
    {
        m_parent = m_current;
        m_current = this;
        m_start = std::chrono::steady_clock::now();
    }

    DebugPerf::~DebugPerf() {
        m_end = std::chrono::steady_clock::now();
        if (m_parent) {
            m_parent->m_childs.emplace_back(*this);
            m_current = m_parent;
        } else {
            m_current = nullptr;
            output_perf_results();
        }
    }

    void DebugPerf::output_perf_results(std::size_t level) const {
        std::string level_str = std::string('\t', level);
        std::chrono::duration<double> diff = m_end - m_start;

        std::cerr << (level == 0 ? "[DEBUG_PERF] " : level_str) << m_name;
        if (!m_childs.empty()) {
            std::cerr << std::endl;
            for (auto const &child : m_childs) {
                child.output_perf_results(level + 1);
            }
            std::cerr << level_str;
        }
        std::cerr << " -- " << diff.count() << "s" << std::endl;
    }
}
#endif
