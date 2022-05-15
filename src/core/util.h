#ifndef _CORE_UTIL_H
#define _CORE_UTIL_H

#include <stddef.h>

template< class Type, ptrdiff_t n >
static ptrdiff_t countOf( Type (&)[n] ) { return n; }

static std::string sanitize_print(const std::string& s) {
    std::string res = s;
    for (auto& c: res) if (c < 0x20) c = '?';
    return res;
}

static std::string sanitize_filename(const std::string& s) {
    std::string res = s;
    for (auto& c: res) if (c<0x20 || c=='/' || c=='\\' || c==':') c = '_';
    return res;
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

#endif // _CORE_UTIL_H
