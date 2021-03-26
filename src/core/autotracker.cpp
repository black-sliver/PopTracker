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
};
