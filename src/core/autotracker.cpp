#include "autotracker.h"
#include "../luaglue/luamethod.h"


const LuaInterface<AutoTracker>::MethodMap AutoTracker::Lua_Methods = {
    LUA_METHOD(AutoTracker, ReadU8, int, int),
    LUA_METHOD(AutoTracker, ReadU16, int, int),
    LUA_METHOD(AutoTracker, ReadU24, int, int),
    LUA_METHOD(AutoTracker, ReadU32, int, int),
    LUA_METHOD(AutoTracker, ReadUInt8, int),
    LUA_METHOD(AutoTracker, ReadUInt16, int),
    LUA_METHOD(AutoTracker, ReadUInt24, int),
    LUA_METHOD(AutoTracker, ReadUInt32, int),
    LUA_METHOD(AutoTracker, ReadVariable, const char*),
};

int AutoTracker::Lua_Index(lua_State *L, const char* key) {
    if (strcmp(key, "SelectedConnectorType")==0) {
        lua_newtable(L); // dummy
        return 1;
    }
    printf("Get AutoTracker.%s unknown\n", key);
    return 0;
}

const std::string AutoTracker::BACKEND_AP_NAME = "AP";
const std::string AutoTracker::BACKEND_UAT_NAME = "UAT";
const std::string AutoTracker::BACKEND_SNES_NAME = "SNES";
const std::string AutoTracker::BACKEND_NONE_NAME = "";
