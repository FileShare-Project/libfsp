/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sat May  6 12:40:55 2023 Francois Michaut
** Last update Sun Jul 16 13:48:35 2023 Francois Michaut
**
** VarInt.hpp : Variable size integer implementation
*/

#include <stdexcept>

#include "FileShare/Utils/VarInt.hpp"

#ifdef DEBUG
  #include <bitset>
  #include <iostream>
#endif

namespace FileShare::Utils {
    VarInt::VarInt(std::size_t value) {
        reset(value);
    }

    VarInt::VarInt(const VarInt &other) {
        *this = other;
    }

    VarInt &VarInt::operator=(const VarInt &other) {
        m_values = other.m_values;
        m_value = other.m_value;
        m_value_dirty = other.m_value_dirty;
        reset_string_view();
        return *this;
    }

    void VarInt::reset(std::size_t value) {
        std::uint8_t tmp;

        m_values.clear();
        m_value_dirty = false;
        m_value = value;
        if (value == 0) {
            m_values.push_back(0);
        }
        while (value != 0) {
            tmp = (value & 0x7F);
            tmp |= (value - tmp == 0 ? 0 : 0x80);
            m_values.push_back(tmp);
            value >>= 7;
        }
        reset_string_view();
    }

    bool VarInt::parse(std::string_view input) {
        std::string_view dummy;

        return parse(input, dummy);
    }

    bool VarInt::parse(std::string_view input, std::string_view &output) {
        std::vector<char> result;

        result.reserve(input.size());
        for (auto iter = input.begin(); iter != input.end(); iter++) {
            result.push_back(*iter);
            // TODO: double check that the fact *iter is signed integer does not mess up the calculations
            if ((*iter & 0x80) == 0) {
                m_value_dirty = true;
                m_values = std::move(result);
                reset_string_view();
                output = std::string_view(iter + 1, input.end());
                return true;
            }
        }
        return false;
    }

    std::string_view VarInt::to_string() const {
        return m_string;
    }

    std::size_t VarInt::to_number() const {
        if (m_value_dirty)
            reset_number_view();
        return m_value;
    }

    void VarInt::reset_string_view() const {
        m_string = std::string_view(m_values.begin(), m_values.end());
    }

    std::size_t VarInt::byte_size() const {
        return m_values.size();
    }

    std::strong_ordering VarInt::operator<=>(const VarInt &other) const {
        // TODO: this won't work for infinite or negative numbers
        return m_value <=> other.m_value;
    }

    void VarInt::reset_number_view() const {
        std::size_t result = 0;
        constexpr std::uint8_t leftover_value = (sizeof(std::size_t) * 8) % 7;
        constexpr std::size_t max_size = (sizeof(std::size_t) * 8) / 7;

        if (m_values.size() > max_size && !(m_values.size() == (max_size + 1) && (std::uint8_t)m_values.back() == leftover_value))
            throw std::runtime_error("Value is too big to fit in a std::size_t");

        // Iterate in reverse since we need the low significant bit first
        // and they are at the end of the string representation
        for (auto iter = m_values.rbegin(); iter != m_values.rend(); iter++) {
            result = (result << 7) + (*iter & 0x7F);
        }
        m_value = result;
        m_value_dirty = false;
    }
#ifdef DEBUG
    void VarInt::debug() const {
        VarInt::debug(m_string);
    }

    void VarInt::debug(std::string_view str) {
        for (char c : str) {
            std::cout << "0b" << std::bitset<8>(c) << " ";
        }
        std::cout << std::endl;
    }
#endif
}
