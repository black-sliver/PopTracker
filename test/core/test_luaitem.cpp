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
