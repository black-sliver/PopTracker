#ifndef _LUASANDBOX_REQUIRE_H
#define _LUASANDBOX_REQUIRE_H

#include <stdio.h>
extern "C" {
#include <lua.h>
#include <lauxlib.h>
}
#include "../core/tracker.h"

static inline int luasandbox_require(lua_State *L)
{
    // get filename being required
    const char* file = luaL_checkstring(L, -1);

    // check cache
    luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_LOADED_TABLE);
    int t = lua_getfield(L, -1, file);
    if (t != LUA_TNONE && t != LUA_TNIL) {
        lua_remove(L, -2); // remove _LOADED from stack
        return 1;
    }
    lua_pop(L, 1); // pop empty cache entry, keep _LOADED

    // get Pack for file from Tracker
    lua_getglobal(L, "Tracker");
    const Tracker* tracker = Tracker::luaL_checkthis(L, -1);
    if (!tracker) {
        lua_pop(L, 2); // pop Tracker and _LOADED
        luaL_error(L, "Wrong type for global 'Tracker'");
        return 0;
    }
    const Pack* pack = tracker->getPack();
    if (!pack) {
        lua_pop(L, 2); // pop Tracker and _LOADED
        luaL_error(L, "No pack loaded");
        return 0;
    }

    // load file to string
    std::string* pscript = new std::string();
    // TODO: add support for package.path
    bool ok = false;
    for (const std::string prefix : {"scripts/", ""}) {
        for (const std::string& folder : {std::string(""), std::string(file)+"/"}) {
            for (const std::string suffix : {"", ".lua"}) {
                ok = pack->ReadFile(prefix + folder + file + suffix, *pscript);
                if (ok) break;
            }
            if (ok) break;
        }
        if (ok) break;
    }
    if (!ok) {
        delete pscript;
        lua_pop(L, 2); // pop Tracker and _LOADED
        luaL_error(L, "No such module: %s", file);
        return 0;
    }
    lua_pop(L, 1); // pop Tracker

    if (luaL_loadbufferx(L, pscript->c_str(), pscript->length(), file, "t") == LUA_OK) {
        if (lua_pcall(L, 0, 1, 0) == LUA_OK) {
            delete pscript;
            lua_pushvalue(L, -1); // duplicate result
            lua_setfield(L, -3, file);
            lua_remove(L, -2); // remove _LOADED from stack
            return 1;
        } else {
            delete pscript;
            lua_remove(L, -2); // remove _LOADED from stack
            lua_error(L);
        }
    } else {
        delete pscript;
        lua_remove(L, -2); // remove _LOADED from stack
        lua_error(L);
    }

    return 0;
}

#endif /* _LUASANDBOX_REQUIRE_H */

