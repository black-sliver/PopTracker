#ifndef _LUAGLUE_LUAREF_H
#define _LUAGLUE_LUAREF_H

extern "C" {
#include <lauxlib.h>
}

/* helper to differenciate between int and ref */
struct LuaRef { int ref=LUA_NOREF; bool valid() const { return ref!=LUA_NOREF; }};

#endif // _LUAGLUE_LUAREF_H
