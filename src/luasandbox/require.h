#ifndef _LUASANDBOX_REQUIRE_H
#define _LUASANDBOX_REQUIRE_H

#include <stdio.h>
#include <luaglue/lua_include.h>
#include "../core/tracker.h"

static inline int luasandbox_require(lua_State *L)
{
    // get filename being required
    const char* name = luaL_checkstring(L, -1);

    // check cache
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    int t = lua_getfield(L, -1, name);
    if (t != LUA_TNONE && t != LUA_TNIL) {
        lua_remove(L, -2); // remove _LOADED from stack
        return 1;
    }
    lua_pop(L, 1); // pop empty cache entry, keep _LOADED

    // get Pack for file from registry
    lua_pushstring(L, "Pack");
    lua_gettable(L, LUA_REGISTRYINDEX); // retrieve stored pointer
    const Pack* pack = static_cast<const Pack*>(lua_touserdata(L, -1));
    if (!pack) {
        lua_pop(L, 2); // pop Tracker and _LOADED
        luaL_error(L, "No pack loaded");
        return 0;
    }

    std::string filename = name;
    std::replace(filename.begin(), filename.end(), '.', '/');

    // load file to string
    std::string pscript;
    // TODO: add support for package.path
    bool ok = false;
    for (const std::string prefix : {"scripts/", ""}) {
        for (const std::string& part : {filename, filename+"/init"}) {
            for (const std::string suffix : {".LUA", ".lua"}) {
                ok = pack->ReadFile(prefix + part + suffix, pscript);
                if (ok) {
                    filename = prefix + part + suffix;
                    break;
                }
            }
            if (ok)
                break;
        }
        if (ok)
            break;
    }
    if (!ok) {
        lua_pop(L, 2); // pop Tracker and _LOADED
        luaL_error(L, "No such module: %s", name);
        return 0;
    }
    lua_pop(L, 1); // pop Tracker

    if (luaL_loadbufferx(L, pscript.c_str(), pscript.length(), filename.c_str(), "t") == LUA_OK) {
        lua_pushstring(L, name);
        if (lua_pcall(L, 1, 1, 0) == LUA_OK) {
            lua_pushvalue(L, -1); // duplicate result
            lua_setfield(L, -3, name);
            lua_remove(L, -2); // remove _LOADED from stack
            return 1;
        } else {
            lua_remove(L, -2); // remove _LOADED from stack
            lua_error(L);
        }
    } else {
        lua_remove(L, -2); // remove _LOADED from stack
        lua_error(L);
    }

    return 0;
}

#endif /* _LUASANDBOX_REQUIRE_H */

