#pragma once

#include <cassert>
#include <gtest/gtest.h>
#include "../../src/ap/archipelago.h"


/// Base to test Archipelago Lua interface while uninitialized
class ArchipelagoLuaNil : public testing::Test {
protected:
    ArchipelagoLuaNil()
        : L(luaL_newstate()), ap(L, nullptr)
    {
        assert(L);
        Archipelago::Lua_Register(L);
    }

    ~ArchipelagoLuaNil() override
    {
        // beware: close (or pop and gc) has to happen before ap goes out of scope
        lua_close(L);
    }

    lua_State* L;
    Archipelago ap;
};

/// Base to test Archipelago Lua interface while disconnected
class ArchipelagoLuaDisconnected : public testing::Test {
protected:
    ArchipelagoLuaDisconnected()
        : L(luaL_newstate()), apTracker("PopTracker"), ap(L, &apTracker)
    {
        assert(L);
        Archipelago::Lua_Register(L);
    }

    ~ArchipelagoLuaDisconnected() override
    {
        // beware: close (or pop and gc) has to happen before ap goes out of scope
        lua_close(L);
    }

    lua_State* L;
    APTracker apTracker;
    Archipelago ap;
};
