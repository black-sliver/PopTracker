#ifndef LUAJSON_H
#define LUAJSON_H

extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <json/json.hpp>
using nlohmann::json;


json lua_to_json(lua_State* L, int n=-1)
{
    json j;
    auto type = lua_type(L,n);
    switch (type) {
        case LUA_TNUMBER:
            if (lua_isinteger(L,n))
                return lua_tointeger(L,n);
            return lua_tonumber(L,n);
        case LUA_TSTRING:
            return lua_tostring(L,n);
        case LUA_TBOOLEAN:
            return (bool)lua_toboolean(L,n);
        case LUA_TTABLE:
        {
            lua_pushnil(L); // first key
            while (lua_next(L,n-1)) {
                // key now at -2, value at -1
                if (lua_isstring(L,-2)) {
                    const char* key = lua_tostring(L,-2);
                    j[key] = lua_to_json(L);
                } else if (lua_isinteger(L,-2)) {
                    // TODO: numbers/arrays
                    fprintf(stderr, "Warning: unhandled type array in lua_to_json()\n");
                }
                // pop value, keep key
                lua_pop(L,1);
            }
            break;
        }
        case LUA_TNIL:
        case LUA_TNONE:
        default:
            // TODO: user type
            break; // return NULL
    }
    return j;
}

void json_to_lua(lua_State* L, json& j)
{
    switch (j.type()) {
        case json::value_t::number_integer:
        case json::value_t::number_unsigned:
            lua_pushinteger(L, j);
            break;
        case json::value_t::number_float:
            lua_pushnumber(L, j);
            break;
        case json::value_t::string:
            lua_pushstring(L, j.get<std::string>().c_str());
            break;
        case json::value_t::object:
            lua_newtable(L);
            for (auto it=j.begin(); it!=j.end(); ++it) {
                json_to_lua(L, it.value());
                lua_setfield(L, -2, it.key().c_str());
            }
            break;
        default:
            // not implemented
            // TODO: array
            fprintf(stderr, "Warning: unhandled type %d %s in json_to_lua()\n",
                    (int)j.type(), j.type_name());
            lua_pushnil(L);
            break;
    }
}

#endif /* LUAJSON_H */

