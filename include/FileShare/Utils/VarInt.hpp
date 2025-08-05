/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 12:40:55 2023 Francois Michaut
** Last update Thu Aug 14 14:20:52 2025 Francois Michaut
**
** VarInt.hpp : Variable size integer (similar to BigInt in the sense that
**              it is infinite, but without arithmetic support)
*/

#pragma once

#include <cstdint>
#include <string>
#include <compare>
#include <vector>

// TODO handle signed integers: https://en.wikipedia.org/wiki/Variable-length_quantity#Signed_numbers
// TODO handle infinite integers
namespace FileShare::Utils {
    class VarInt {
        public:
            VarInt() = default;
            VarInt(std::size_t);

            VarInt(const VarInt &);
            VarInt(VarInt &&) = default;

            auto operator=(const VarInt &) -> VarInt &;
            auto operator=(VarInt &&) -> VarInt & = default;

            void reset(std::size_t);
            auto parse(std::string_view input) -> bool;
            auto parse(std::string_view input, std::string_view &output) -> bool;

            auto to_string() const -> std::string_view { return m_string; }
            auto to_number() const -> std::size_t;
            auto byte_size() const -> std::size_t { return m_values.size(); }

#ifdef DEBUG
            void debug() const;
            static void debug(std::string_view str);
#endif

            auto operator<=>(const VarInt &other) const -> std::strong_ordering;
            auto operator==(const VarInt &other) const -> bool = default;
        private:
            void reset_string_view() const;
            void reset_number_view() const;

            std::vector<char> m_values;

            mutable std::string_view m_string;
            mutable std::size_t m_value;
            mutable bool m_value_dirty = true;
    };
}
