#include <gtest/gtest.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <memory>


static std::weak_ptr<int> ptr;

static int func(lua_State *L) {
    auto mem = std::make_shared<int>(42);
    ptr = mem;
    asm volatile("": : :"memory");
    luaL_error(L, "error");
    return 0;
}

TEST(LuaTest, CppStackUnwrap) {
    // Lua needs to be configured to use C++ exceptions instead of longjmp for this to succeed
    auto initial = std::make_shared<int>(0);
    ptr = initial;

    lua_State* L = luaL_newstate();
    ASSERT_TRUE(L);
    lua_pushcfunction(L, func);
    lua_setglobal(L, "func");
    luaL_loadstring(L, "func()");
    auto res = lua_pcall(L, 0, 0, 0);
    EXPECT_NE(res, 0) << "Lua func did not throw";
    lua_close(L);

    if (auto p = ptr.lock()) {
        ASSERT_EQ(*p, 42) << "Lua func was never run";
    }
    EXPECT_TRUE(ptr.expired()) << "Lua func did not properly unwrap the stack";
}

TEST(LuaTest, AssignToItorator) {
    // This is a breaking change for many packs in the preview of Lua 5.5
    // not sure what to do once 5.4 reaches EOL
    auto script = R""""(
        local table = {a = "x", b = "y", c = "z"}
        local n = 0
        for k, v in pairs(table) do
            k = k .. ":"  -- this assignment fails in Lua 5.5 preview
            n = n + 1
        end
        assert(n == 3, "Iteration did not succeed")
    )"""";

    lua_State* L = luaL_newstate();
    ASSERT_TRUE(L);
    luaL_requiref(L, LUA_GNAME, luaopen_base, 1);
    lua_pop(L, 1);
    luaL_loadstring(L, script);
    auto res = lua_pcall(L, 0, 0, 0);
    EXPECT_EQ(res, 0) << lua_tostring(L, -1);
    lua_close(L);
}
