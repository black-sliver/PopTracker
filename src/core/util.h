#ifndef _CORE_UTIL_H
#define _CORE_UTIL_H

#include <algorithm>
#include <cstddef>
#include <cstring>
#include "fs.h"

#ifdef __has_include
#  if __has_include(<utility>)
#    include <utility>
#  endif
#endif


template< class Type, ptrdiff_t n >
static ptrdiff_t countOf( Type (&)[n] ) { return n; }

static std::string sanitize_print(const std::string& s) {
    std::string res = s;
    for (auto& c: res) if (c < 0x20) c = '?';
    return res;
}

static std::string sanitize_print(const char* s)
{
    return sanitize_print(std::string(s));
}

static std::string sanitize_print(const fs::path& path)
{
    return sanitize_print(path.u8string());
}

/// Replaces reserved/non-portable symbols by '_'.
static std::string sanitize_filename(std::string s) {
    auto exclude = "<>:\"/\\|?*$'`";
    auto sanitize = [&](char c) { return strchr(exclude, c) || c == 0; };
    std::replace_if(s.begin(), s.end(), sanitize, '_');
    return s;
}

/// Replaces non-ASCII and reserved/non-portable symbols by '_'. Returns "_" for empty and reserved folder names.
static std::string sanitize_dir(std::string s)
{
    if (s.empty())
        return "_";
    if ((size_t)std::count(s.begin(), s.end(), '.') == s.length())
        return "_";
    auto exclude = "<>:\"/\\|?*$'`";
    auto sanitize = [&](char c) { return c < 0x20 || c >= 0x7f || strchr(exclude, c); };
    std::replace_if(s.begin(), s.end(), sanitize, '_');
    return s;
}

template<typename T>
std::string format_bytes(T val) {
    const char suffix[] = "KMGT";
    ssize_t n = -1;
    if (std::is_integral<T>::value) {
        val *= 10; // integer with 1 decimal place
        while (val>=10240 && n<(ssize_t)strlen(suffix)-1) {
            val = (val+512) / 1024;
            n++;
        }
        if (val>999) {
            val = ((val+5)/10)*10; // remove/round decimal place for >= 3digits
        }
    } else {
        while (val>=1024 && n<(ssize_t)strlen(suffix)-1) {
            val /= 1024;
            n++;
        }
    }
    std::string s;
    if (std::is_integral<T>::value) {
        if (val>999) s = std::to_string(val/10);
        else s = std::to_string(val/10) + "." + std::to_string(val%10);
    } else {
        s = std::to_string(val);
    }
    if (n>=0) s += std::string(1, suffix[n]);
    return s;
}

template<typename T>
unsigned upercent(T divident, T divisor)
{
    static_assert(sizeof(unsigned) >= 4, "Unsupported platform");
    while (divident > 42949672) {
        // avoid overflow during integer multiplication
        divident = (divident + 5) / 10;
        divisor = (divisor + 5) / 10;
    }
    return 100U * divident / divisor;
}

static void strip(std::string& s, const char* whitespace = " \t\r\n")
{
    if (!s.empty()) {
        auto start = s.find_first_not_of(whitespace);
        if (start == s.npos) {
            s.clear();
        } else {
            auto end = s.find_last_not_of(whitespace);
            s = s.substr(start, end - start + 1);
        }
    }
}

namespace util {
#ifdef __cpp_lib_unreachable
    using unreachable = std::unreachable;
#else
    [[noreturn]] static inline void unreachable()
    {
#    if defined(_MSC_VER) && !defined(__clang__) // MSVC
        __assume(false);
#    else // GCC, Clang
        __builtin_unreachable();
#    endif
    }
#endif
}

#endif // _CORE_UTIL_H
