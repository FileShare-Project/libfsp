/*
** Project LibFileShareProtocol, 2025
**
** Author Francois Michaut
**
** Started on  Sun Aug 10 12:20:05 2025 Francois Michaut
** Last update Mon Aug 18 16:30:01 2025 Francois Michaut
**
** Vector.hpp : Utility functions for std::vector
*/

#include <vector>

namespace FileShare {
    template<class Vec, typename Iter = typename Vec::iterator>
    auto delete_move(Vec &vector, Iter iter) -> Iter {
        auto back = std::prev(vector.end());

        if (iter == back) {
            vector.pop_back();
            return vector.end();
        }

        *iter = std::move(*back);
        vector.pop_back();
        return iter;
    }
}
