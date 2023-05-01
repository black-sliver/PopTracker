#ifndef _LUAGLUE_LUAVARIANT_H
#define _LUAGLUE_LUAVARIANT_H

#include "luatype.h"
#include "luacompat.h"
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <string>

class LuaVariant final : public LuaType {
private:
    #define LUA_TINTEGER 0x100 // this does not actually exist in lua
    int type = LUA_TNONE;
    union {
        lua_Number number;
        int boolean;
        lua_Integer integer;
    };
    std::string string;
    
public:
    bool operator==(const LuaVariant& other) const {
        if (type != other.type) return false;
        if (type == LUA_TNIL || type == LUA_TNONE) return true;
        if (type == LUA_TINTEGER) return integer==other.integer;
        if (type == LUA_TNUMBER) return number==other.number;
        if (type == LUA_TBOOLEAN) return boolean==other.boolean;
        if (type == LUA_TSTRING) return string==other.string;
        return false;
    }
    bool Lua_Get(lua_State *L, int i) {
        type = lua_type(L, i);
        switch (type) {
            case LUA_TNUMBER:
                if (lua_isinteger(L, i)) {
                    integer = lua_tointeger(L, i);
                    type = LUA_TINTEGER;
                } else {
                    number = lua_tonumber(L, i);
                }
                break;
            case LUA_TSTRING:
                string = lua_tostring(L, i);
                break;
            case LUA_TBOOLEAN:
                boolean = lua_toboolean(L, i);
                break;
            case LUA_TNIL:
                break;
            case LUA_TNONE:
                return false;
            default:
                // TODO: user type
                return false;
        }
        return true;
    }
    void Lua_Push(lua_State *L) const {
        switch (type) {
            case LUA_TINTEGER:
                lua_pushinteger(L, integer);
                break;
            case LUA_TNUMBER:
                lua_pushnumber(L, number);
                break;
            case LUA_TBOOLEAN:
                lua_pushboolean(L, boolean);
                break;
            case LUA_TSTRING:
                lua_pushstring(L, string.c_str());
                break;
            case LUA_TNIL:
                lua_pushnil(L);
                break;
            case LUA_TNONE:
                break;
            default:
                // TODO: user type
                break;
        }
    }
    std::string toString() const {
        switch (type) {
            case LUA_TINTEGER:
                return std::to_string(integer);
            case LUA_TNUMBER:
                return std::to_string(number);
            case LUA_TBOOLEAN:
                return boolean ? "true" : "false";
            case LUA_TSTRING:
                return "\""+string+"\"";
            case LUA_TNIL:
                return "nil";
            case LUA_TNONE:
                return "";
            default:
                // TODO: user type
                return "<not implemented>";
        }
    }

    bool isFalse() const {
        return type == LUA_TBOOLEAN && !boolean;
    }

    bool isTrue() const {
        return type == LUA_TBOOLEAN && boolean;
    }
};


#endif /* _LUAGLUE_LUAVARIANT_H */

