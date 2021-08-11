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

#endif // _CORE_UTIL_H
