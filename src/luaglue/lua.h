#ifndef _LUAGLUE_LUA_H
#define _LUAGLUE_LUA_H

#include "luatype.h"
#include "luaref.h"
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}


// Note: we basically wrap lua just to have parameter overloading
// we may want to actually fully wrap lua to manage the life cycle?
class Lua final {
public:
    Lua(lua_State *L) : L(L) {}
    
    template<class T>
    void Register()
    {
        T::Lua_Register(L);
    }
    
    void Push(const LuaType& o)
    {
        o.Lua_Push(L);
    }
    void Push(const LuaType* o)
    {
        o->Lua_Push(L);
    }
    void Push(const char* s)
    {
        lua_pushstring(L, s);
    }
    void Push(const std::string& s)
    {
        lua_pushstring(L, s.c_str());
    }
    void Push(lua_Integer i)
    {
        lua_pushinteger(L, i);
    }
    void Push(lua_Number n)
    {
        lua_pushnumber(L, n);
    }
    void Push(bool b)
    {
        lua_pushboolean(L, b);
    }
    void PushNil()
    {
        lua_pushnil(L);
    }
    void Push(std::nullptr_t)
    {
        lua_pushnil(L);
    }
private:
    lua_State *L;
};


#endif // _LUAGLUE_LUA_H
