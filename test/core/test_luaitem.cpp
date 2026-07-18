#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <luaglue/luapp.h>
#include <nlohmann/json.hpp>
#include "../../src/core/luaitem.h"
#include "../../src/core/pack.h"
#include "../../src/core/scripthost.h"
#include "../../src/core/tracker.h"


class LuaItemMockEventHandler {
public:
    MOCK_METHOD(void, onEvent, ());
};

TEST(LuaItemProvidesTest, Simple) {
    Pack pack("examples/async"); // doesn't matter which one
    lua_State* L = luaL_newstate();
    ASSERT_TRUE(L);

    Tracker tracker(&pack, L);
    Tracker::Lua_Register(L);
    tracker.Lua_Push(L);
    lua_setglobal(L, "Tracker");
    lua_pushstring(L, "Pack");
    lua_pushlightuserdata(L, &pack);
    lua_settable(L, LUA_REGISTRYINDEX);
    ScriptHost scriptHost(&pack, L, &tracker);
    ScriptHost::Lua_Register(L);
    scriptHost.Lua_Push(L);
    lua_setglobal(L, "ScriptHost");
    LuaItem::Lua_Register(L);

    const char* script = R"(
        local item = ScriptHost:CreateLuaItem()
        item.Name = "test"
        local active = false
        function item:CanProvideCodeFunc(code)
            return code == "Code"
        end
        function item:ProvidesCodeFunc(code)
            return active and (code == "Code") and 1 or 0
        end
        function item:OnLeftClickFunc()
            active = true
            self.Icon = "active"  -- trigger onChange
        end
    )";
    const char* modName = "script";
    ASSERT_EQ(luaL_loadbufferx(L, script, strlen(script), modName, "t"), LUA_OK);
    lua_pushstring(L, modName);
    ASSERT_EQ(lua_pcall(L, 1, 1, 0), LUA_OK) << lua_tostring(L, -1);

    auto& item = tracker.getItemById(tracker.getItemByCode("Code").getID());

    LuaItemMockEventHandler mock;
    item.onChange += {&mock, [&mock](void*) {mock.onEvent();}};
    EXPECT_CALL(mock, onEvent()).Times(1);

    EXPECT_TRUE(item.canProvideCode("Code"));
    EXPECT_FALSE(item.canProvideCode("Not"));
    EXPECT_EQ(item.providesCode("Code"), 0);
    EXPECT_TRUE(item.changeState(BaseItem::Action::Primary));
    EXPECT_EQ(item.providesCode("Code"), 1);

    lua_close(L);
}

TEST(LuaItemProvidesTest, CanProvideCodeIsCached) {
    Pack pack("examples/async"); // doesn't matter which one
    lua_State* L = luaL_newstate();
    ASSERT_TRUE(L);

    Tracker tracker(&pack, L);
    Tracker::Lua_Register(L);
    tracker.Lua_Push(L);
    lua_setglobal(L, "Tracker");
    lua_pushstring(L, "Pack");
    lua_pushlightuserdata(L, &pack);
    lua_settable(L, LUA_REGISTRYINDEX);
    ScriptHost scriptHost(&pack, L, &tracker);
    ScriptHost::Lua_Register(L);
    scriptHost.Lua_Push(L);
    lua_setglobal(L, "ScriptHost");
    LuaItem::Lua_Register(L);

    const char* script = R"(
        calls = 0
        local clicks = 0
        local item = ScriptHost:CreateLuaItem()
        item.Name = "test"
        function item:CanProvideCodeFunc(code)
            calls = calls + 1
            return code == "Code"
        end
        function item:ProvidesCodeFunc(code)
            return (code == "Code") and 1 or 0
        end
        function item:OnLeftClickFunc()
            clicks = clicks + 1
            self.Icon = "icon" .. clicks  -- trigger onChange
        end
        function makeLateItem()
            lateItem = ScriptHost:CreateLuaItem()
            lateItem.Name = "late"
        end
        function giveLateItemCodes()
            function lateItem:CanProvideCodeFunc(code)
                return code == "Late"
            end
            function lateItem:ProvidesCodeFunc(code)
                return (code == "Late") and 1 or 0
            end
        end
        function makeSecondItem()
            local other = ScriptHost:CreateLuaItem()
            other.Name = "other"
            function other:CanProvideCodeFunc(code)
                return code == "Other"
            end
            function other:ProvidesCodeFunc(code)
                return (code == "Other") and 1 or 0
            end
        end
    )";
    const char* modName = "script";
    ASSERT_EQ(luaL_loadbufferx(L, script, strlen(script), modName, "t"), LUA_OK);
    lua_pushstring(L, modName);
    ASSERT_EQ(lua_pcall(L, 1, 1, 0), LUA_OK) << lua_tostring(L, -1);

    auto calls = [L]() {
        lua_getglobal(L, "calls");
        const int n = (int)lua_tointeger(L, -1);
        lua_pop(L, 1);
        return n;
    };

    EXPECT_EQ(tracker.ProviderCountForCode("Code"), 1);
    EXPECT_EQ(tracker.ProviderCountForCode("Other"), 0);
    const int afterFirst = calls();
    EXPECT_GT(afterFirst, 0);

    // a state change drops _providerCountCache, but must not make us ask Lua again
    auto& item = tracker.getItemById(tracker.getItemByCode("Code").getID());
    EXPECT_TRUE(item.changeState(BaseItem::Action::Primary));
    EXPECT_EQ(tracker.ProviderCountForCode("Code"), 1);
    EXPECT_EQ(tracker.ProviderCountForCode("Other"), 0);
    EXPECT_EQ(calls(), afterFirst);

    // adding an item has to invalidate the cache
    lua_getglobal(L, "makeSecondItem");
    ASSERT_EQ(lua_pcall(L, 0, 0, 0), LUA_OK) << lua_tostring(L, -1);
    EXPECT_TRUE(item.changeState(BaseItem::Action::Primary));
    EXPECT_EQ(tracker.ProviderCountForCode("Other"), 1);
    EXPECT_GT(calls(), afterFirst);

    // an item that declares its codes after creation must invalidate too
    lua_getglobal(L, "makeLateItem");
    ASSERT_EQ(lua_pcall(L, 0, 0, 0), LUA_OK) << lua_tostring(L, -1);
    EXPECT_EQ(tracker.ProviderCountForCode("Late"), 0);
    lua_getglobal(L, "giveLateItemCodes");
    ASSERT_EQ(lua_pcall(L, 0, 0, 0), LUA_OK) << lua_tostring(L, -1);
    EXPECT_EQ(tracker.ProviderCountForCode("Late"), 1);

    lua_close(L);
}
