#include <cassert>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <sltbench/BenchCore.h>


void BenchLua()
{
    // Simple benchmark that sets up lua and runs some random code to compare build flag performance
    auto script = R""""(
        local x = 0;
        for i=1,10000 do
            x = x + 1
        end
    )"""";

    lua_State* L = luaL_newstate();
    luaL_loadstring(L, script);
    assert(lua_pcall(L, 0, 0, 0) == LUA_OK);
    lua_close(L);
}
SLTBENCH_FUNCTION(BenchLua);
