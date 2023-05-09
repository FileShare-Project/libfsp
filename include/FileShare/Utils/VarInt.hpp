/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 12:40:55 2023 Francois Michaut
** Last update Sun May 14 14:48:15 2023 Francois Michaut
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

            VarInt &operator=(const VarInt &);
            VarInt &operator=(VarInt &&) = default;

            void reset(std::size_t);
            bool parse(std::string_view input);
            bool parse(std::string_view input, std::string_view &output);

            std::string_view to_string() const;
            std::size_t to_number() const;
            std::size_t byte_size() const;

#ifdef DEBUG
            void debug() const;
            static void debug(std::string_view str);
#endif

            std::strong_ordering operator<=>(const VarInt &other) const;
            bool operator==(const VarInt &other) const = default;
        private:
            void reset_string_view() const;
            void reset_number_view() const;

            std::vector<char> m_values;

            mutable std::string_view m_string;
            mutable std::size_t m_value;
            mutable bool m_value_dirty = true;
    };
}
