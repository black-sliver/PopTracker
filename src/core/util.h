#ifndef _CORE_UTIL_H
#define _CORE_UTIL_H

#include <stddef.h>

template< class Type, ptrdiff_t n >
ptrdiff_t countOf( Type (&)[n] ) { return n; }

#endif // _CORE_UTIL_H
