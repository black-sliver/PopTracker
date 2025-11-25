// NOTE: we don't have a mock AP server yet, so this just visits the not-connected code path

#include <gtest/gtest.h>
#include "lua_fixtures.hpp"
#include "../../lib/lua/lua.h"


/// Test that Archipelago.Seed gives nil for non-AP-packs
TEST_F(ArchipelagoLuaNil, Seed) {
    ap.Lua_Push(L);
    lua_getfield(L, -1, "Seed");
    EXPECT_EQ(2, lua_gettop(L));
    EXPECT_EQ(LUA_TNIL, lua_type(L, -1));
}

/// Test that Archipelago.Seed gives nil while not connected
TEST_F(ArchipelagoLuaDisconnected, Seed) {
    ap.Lua_Push(L);
    lua_getfield(L, -1, "Seed");
    EXPECT_EQ(2, lua_gettop(L));
    EXPECT_EQ(LUA_TNIL, lua_type(L, -1));
}
