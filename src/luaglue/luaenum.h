#ifndef _LUAGLUE_LUAENUM_H
#define _LUAGLUE_LUAENUM_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <map> // TODO: write a constexpr_map for MethodMap instead
#include <string>

template <typename T>
class LuaEnum {
public:
    LuaEnum(std::map<std::string, T>&& values)
            : _values(values)
    {
    }

    void Lua_Push(lua_State *L)
    {
        lua_createtable(L, 0, _values.size());
        for (const auto& pair: _values)
        {
            lua_pushstring(L, pair.first.c_str());
            lua_pushinteger(L, (lua_Integer)pair.second);
            lua_settable(L, -3);
        }
    }

    void Lua_SetGlobal(lua_State *L, const char* name)
    {
        Lua_Push(L);
        lua_setglobal(L, name);
    }

protected:
    std::map<std::string, T> _values;
};

#endif /* _LUAGLUE_LUAENUM_H */

