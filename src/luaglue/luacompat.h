#ifndef _LUAGLUE_LUACOMPAT_H
#define _LUAGLUE_LUACOMPAT_H


// IMPORTANT:
// This compatibility layer is compile-time, so you will have to target the correct version of Lua when compiling.
// Stuff like __attribute((weak)) and /alternatename is not cross-platform, so we ditch that idea for now.


extern "C" {
#include <lua.h>
#include <lauxlib.h>
}
#include <limits>


#if LUA_VERSION_NUM < 503
static int lua_isinteger(lua_State *L, int idx)
{
    if (!lua_isnumber(L, idx))
        return 0;
    lua_Number number = lua_tonumber(L, idx);
    if (number < std::numeric_limits<lua_Integer>::min() || number > std::numeric_limits<lua_Integer>::max())
        return 0;
    return trunc(number) == number;    
}
#endif

#if LUA_VERSION_NUM < 502
// NOTE: lua_objlen was renamed to lua_len in 5.2, so we can't use the same code everywhere
static lua_Integer luaL_len(lua_State *L, int idx)
{
    return (lua_Integer)lua_objlen(L, idx);
}
#endif

#ifndef LUA_OK
#define LUA_OK 0
#endif


#endif // _LUAGLUE_LUACOMPAT_H
