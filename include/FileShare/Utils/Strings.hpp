/*
** Project LibFileShareProtocol, 2023
**
** Author Francois Michaut
**
** Started on  Sun Nov 19 13:31:02 2023 Francois Michaut
** Last update Mon Aug 11 10:24:57 2025 Francois Michaut
**
** Strings.hpp : Utility classes for string+string view
*/

#pragma once

#include <cwctype>
#include <filesystem>
#include <string>

namespace FileShare::Utils {
    struct string_hash {
        using hash_type = std::hash<std::string_view>;
        using is_transparent = void;

        auto operator()(const char* str) const -> std::size_t        { return hash_type{}(str); }
        auto operator()(std::string_view str) const -> std::size_t   { return hash_type{}(str); }
        auto operator()(const std::string &str) const -> std::size_t { return hash_type{}(str); }
        auto operator()(const std::filesystem::path &str) const -> std::size_t { return hash_type{}(str.string().c_str()); }
    };

    template <class CharT>
    requires(sizeof(CharT) <= sizeof(wchar_t))
    struct ci_char_traits : public std::char_traits<CharT> {
        public:
            static auto eq(CharT char1, CharT char2) -> bool { return to_lower(char1) == to_lower(char2); }
            static auto ne(CharT char1, CharT char2) -> bool { return to_lower(char1) != to_lower(char2); }
            static auto lt(CharT char1, CharT char2) -> bool { return to_lower(char1) <  to_lower(char2); }

            static auto compare(const CharT* str1, const CharT* str2, size_t n) -> int {
                while (n-- != 0) {
                    CharT char1 = to_lower(*str1);
                    CharT char2 = to_lower(*str2);

                    if (char1 < char2) {
                        return -1;
                    }
                    if (char1 > char2) {
                        return 1;
                    }

                    ++str1;
                    ++str2;
                }
                return 0;
            }

            static auto find(const CharT* str, std::size_t n, CharT chr) -> const CharT* {
                while (n-- > 0 && to_lower(*str) != to_lower(chr)) {
                    ++str;
                }
                return str;
            }

        private:
            constexpr static auto to_lower(CharT chr) -> CharT {
                if constexpr (sizeof(CharT) <= sizeof(char)) {
                    return static_cast<CharT>(std::tolower(static_cast<unsigned char>(chr)));
                } else {
                    return static_cast<CharT>(std::towlower(static_cast<wchar_t>(chr)));
                }
            }
    };

    template<class CharT>
    using basic_ci_string = std::basic_string<CharT, ci_char_traits<CharT>>;

    template<class CharT>
    auto operator>>(std::istream &is, basic_ci_string<CharT> &obj) -> std::istream & {
        return is >> reinterpret_cast<std::basic_string<CharT> &>(obj);
    }

    template<class CharT>
    auto operator<<(std::ostream &os, const basic_ci_string<CharT> &obj) -> std::ostream & {
        return os << reinterpret_cast<const std::basic_string<CharT> &>(obj);
    }

    template<class CharT>
    auto operator==(const basic_ci_string<CharT> &lhs, const std::basic_string<CharT> &rhs) noexcept -> bool {
        return lhs == reinterpret_cast<const basic_ci_string<CharT> &>(rhs);
    }

    // TODO: Add < > <= >= != if needed, and complete the following ones :

    // template<class CharT>
    // basic_ci_string<CharT> operator+(const std::basic_string<CharT> &lhs, const std::basic_string<CharT> &rhs) {
    // }

    // template<class CharT>
    // ??? operator+(const std::basic_string<CharT> &lhs, const std::basic_string<CharT> &rhs) {
    // }

    // template<class CharT>
    // void swap(basic_ci_string<CharT> &lhs, basic_ci_string<CharT> &rhs);

    // template<class CharT>
    // void swap(basic_ci_string<CharT> &lhs, std::basic_string<CharT> &rhs);

    // template<class CharT, class Traits>
    // std::basic_istream<CharT, Traits> &getline(std::basic_istream<CharT, Traits> &&input, basic_ci_string<CharT> &str, CharT delim = '\n');

    template<class CharT>
    using basic_ci_string_view = std::basic_string_view<CharT, ci_char_traits<CharT>>;

    using ci_string = basic_ci_string<char>;
    // NOTE: Theses are completely untested.
    using wci_string = basic_ci_string<wchar_t>;
    using u8ci_string = basic_ci_string<char8_t>;
    using u16ci_string = basic_ci_string<char16_t>;
    using u32ci_string = basic_ci_string<char32_t>;

    using ci_string_view = basic_ci_string_view<char>;
    // NOTE: Theses are completely untested.
    using wci_string_view = basic_ci_string_view<wchar_t>;
    using u8ci_string_view = basic_ci_string_view<char8_t>;
    using u16ci_string_view = basic_ci_string_view<char16_t>;
    using u32ci_string_view = basic_ci_string_view<char32_t>;
}
