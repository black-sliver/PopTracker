// NOTE: we don't have a mock AP server yet, so this just visits the not-connected code path

#include <gtest/gtest.h>
#include "lua_fixtures.hpp"
#include "../../lib/lua/lua.h"


/// Test that Archipelago.PlayerNumber gives -1 for non-AP-packs
TEST_F(ArchipelagoLuaNil, PlayerNumber) {
    ap.Lua_Push(L);
    lua_getfield(L, -1, "PlayerNumber");
    EXPECT_EQ(2, lua_gettop(L));
    EXPECT_EQ(-1, lua_tointeger(L, -1));
}

/// Test that Archipelago.PlayerNumber gives -1 while not connected
TEST_F(ArchipelagoLuaDisconnected, PlayerNumber) {
    ap.Lua_Push(L);
    lua_getfield(L, -1, "PlayerNumber");
    EXPECT_EQ(2, lua_gettop(L));
    EXPECT_EQ(-1, lua_tointeger(L, -1));
}
