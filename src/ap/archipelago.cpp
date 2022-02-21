#include "archipelago.h"
#include "../luaglue/luamethod.h"
#include "../luaglue/lua.h"
#include "../luaglue/lua_json.h"


const LuaInterface<Archipelago>::MethodMap Archipelago::Lua_Methods = {
    LUA_METHOD(Archipelago, AddClearHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddItemHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddLocationHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddScoutHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddBouncedHandler, const char*, LuaRef),
};

Archipelago::Archipelago(lua_State *L, APTracker *ap)
    : _L(L), _ap(ap)
{
}

Archipelago::~Archipelago()
{
    _L = nullptr;
    _ap = nullptr;
}

bool Archipelago::AddClearHandler(const std::string& name, LuaRef callback)
{
    if (!_ap || !callback.valid()) return false;
    int ref = callback.ref;
    _ap->onClear += {this, [this, ref, name](void*, const json& slotData) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        json_to_lua(_L, slotData);
        if (lua_pcall(_L, 1, 0, 0)) {
            const char* err = lua_tostring(_L, -1);
            printf("Error calling Archipelago ClearHandler for %s: %s\n",
                    name.c_str(), err ? err : "Unknown");
            lua_pop(_L, 1);
        }
    }};
    return true;
}

bool Archipelago::AddItemHandler(const std::string& name, LuaRef callback)
{
    if (!_ap || !callback.valid()) return false;
    int ref = callback.ref;
    _ap->onItem += {this, [this, ref, name](void*, int index, int item, const std::string& item_name) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        Lua(_L).Push((lua_Integer)index);
        Lua(_L).Push((lua_Integer)item);
        Lua(_L).Push(item_name.c_str());
        if (lua_pcall(_L, 3, 0, 0)) {
            const char* err = lua_tostring(_L, -1);
            printf("Error calling Archipelago ItemHandler for %s: %s\n",
                    name.c_str(), err ? err : "Unknown");
            lua_pop(_L, 1);
        }
    }};
    return true;
}

bool Archipelago::AddLocationHandler(const std::string& name, LuaRef callback)
{
    if (!_ap || !callback.valid()) return false;
    int ref = callback.ref;
    _ap->onLocationChecked += {this, [this, ref, name](void*, int location, const std::string& location_name) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        Lua(_L).Push((lua_Integer)location);
        Lua(_L).Push(location_name.c_str());
        if (lua_pcall(_L, 2, 0, 0)) {
            const char* err = lua_tostring(_L, -1);
            printf("Error calling Archipelago LocationHandler for %s: %s\n",
                    name.c_str(), err ? err : "Unknown");
            lua_pop(_L, 1);
        }
    }};
    return true;
}

bool Archipelago::AddScoutHandler(const std::string& name, LuaRef callback)
{
    if (!_ap || !callback.valid()) return false;
    int ref = callback.ref;
    _ap->onScout += {this, [this, ref, name](void*, int location, const std::string& location_name,
            int item, const std::string& item_name, int player) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        Lua(_L).Push((lua_Integer)location);
        Lua(_L).Push(location_name.c_str());
        Lua(_L).Push((lua_Integer)item);
        Lua(_L).Push(item_name.c_str());
        Lua(_L).Push((lua_Integer)player);
        if (lua_pcall(_L, 5, 0, 0)) {
            const char* err = lua_tostring(_L, -1);
            printf("Error calling Archipelago ScoutHandler for %s: %s\n",
                    name.c_str(), err ? err : "Unknown");
            lua_pop(_L, 1);
        }
    }};
    return true;
}

bool Archipelago::AddBouncedHandler(const std::string& name, LuaRef callback)
{
    if (!_ap || !callback.valid()) return false;
    int ref = callback.ref;
    _ap->onBounced += {this, [this, ref, name](void*, const json& data) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        json_to_lua(_L, data);
        if (lua_pcall(_L, 1, 0, 0)) {
            const char* err = lua_tostring(_L, -1);
            printf("Error calling Archipelago BouncedHandler for %s: %s\n",
                    name.c_str(), err ? err : "Unknown");
            lua_pop(_L, 1);
        }
    }};
    return true;
}

int Archipelago::Lua_Index(lua_State *L, const char* key) {
    if (strcmp(key, "PlayerNumber")==0) {
        lua_pushinteger(L, _ap ? _ap->getPlayerNumber() : -1);
        return 1;
    }
    printf("Get Archipelago.%s unknown\n", key);
    return 0;
}