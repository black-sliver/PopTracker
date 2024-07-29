#include "archipelago.h"
#include <limits>
#include <luaglue/luamethod.h>
#include <luaglue/luapp.h>
#include <luaglue/lua_json.h>


const LuaInterface<Archipelago>::MethodMap Archipelago::Lua_Methods = {
    LUA_METHOD(Archipelago, AddClearHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddItemHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddLocationHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddScoutHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddBouncedHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddRetrievedHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, AddSetReplyHandler, const char*, LuaRef),
    LUA_METHOD(Archipelago, SetNotify, json),
    LUA_METHOD(Archipelago, Get, json),
    LUA_METHOD(Archipelago, LocationChecks, json),
    LUA_METHOD(Archipelago, LocationScouts, json, int),
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
    _ap->onItem += {this, [this, ref, name](void*, int index, int64_t item, const std::string& item_name, int player) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        Lua(_L).Push(index);
        Lua(_L).Push(item);
        Lua(_L).Push(item_name.c_str());
        Lua(_L).Push(player);
        if (lua_pcall(_L, 4, 0, 0)) {
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
    _ap->onLocationChecked += {this, [this, ref, name](void*, int64_t location, const std::string& location_name) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        Lua(_L).Push(location);
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
    _ap->onScout += {this, [this, ref, name](void*, int64_t location, const std::string& location_name,
            int64_t item, const std::string& item_name, int player) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        Lua(_L).Push(location);
        Lua(_L).Push(location_name.c_str());
        Lua(_L).Push(item);
        Lua(_L).Push(item_name.c_str());
        Lua(_L).Push(player);
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


bool Archipelago::AddRetrievedHandler(const std::string& name, LuaRef callback)
{
    if (!_ap || !callback.valid()) return false;
    int ref = callback.ref;
    _ap->onRetrieved += {this, [this, ref, name](void*, const std::string& key, const json& value) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        Lua(_L).Push(key.c_str());
        json_to_lua(_L, value);
        if (lua_pcall(_L, 2, 0, 0)) {
            const char* err = lua_tostring(_L, -1);
            printf("Error calling Archipelago RetrievedHandler for %s: %s\n",
                    name.c_str(), err ? err : "Unknown");
            lua_pop(_L, 1);
        }
    }};
    return true;
}

bool Archipelago::AddSetReplyHandler(const std::string& name, LuaRef callback)
{
    if (!_ap || !callback.valid()) return false;
    int ref = callback.ref;
    _ap->onSetReply += {this, [this, ref, name](void*, const std::string& key, const json& value, const json& old) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        Lua(_L).Push(key.c_str());
        json_to_lua(_L, value);
        json_to_lua(_L, old);
        if (lua_pcall(_L, 3, 0, 0)) {
            const char* err = lua_tostring(_L, -1);
            printf("Error calling Archipelago SetReplyHandler for %s: %s\n",
                    name.c_str(), err ? err : "Unknown");
            lua_pop(_L, 1);
        }
    }};
    return true;
}

bool Archipelago::SetNotify(const json& jKeys)
{
    if (!_ap || (!jKeys.is_array() && !jKeys.is_object())) {
        printf("Archipelago.SetNotify: keys must be array/table!\n");
        return false;
    }
    std::list<std::string> keys;
    for (auto& el: jKeys.items()) {
        if (el.value().is_string()) {
            keys.push_back(el.value());
        } else {
            printf("Archipelago.SetNotify: keys must be array/table of string!\n");
            return false;
        }
    }
    return _ap->SetNotify(keys);
}

bool Archipelago::Get(const json& jKeys)
{
    if (!_ap || (!jKeys.is_array() && !jKeys.is_object())) {
        printf("Archipelago.Get: keys must be array/table!\n");
        return false;
    }
    std::list<std::string> keys;
    for (auto& el: jKeys.items()) {
        if (el.value().is_string()) {
            keys.push_back(el.value());
        } else {
            printf("Archipelago.Get: keys must be array/table of string!\n");
            return false;
        }
    }
    return _ap->Get(keys);
}

bool Archipelago::LocationChecks(const json &jLocations)
{
    if (!_ap)
        return false; //throw std::runtime_error("Not connected");
    if (!_ap->allowSend())
        return false; //throw std::runtime_error("Tracker-only");
    if ((!jLocations.is_array() && !jLocations.is_object()))
        return false; //throw std::runtime_error("argument locations to LocationChecks must be array/table");

    std::list<std::int64_t> locations;
    for (auto& el: jLocations.items()) {
        if (el.value().is_number_integer()) {
            locations.push_back(el.value());
        } else {
            return false;
        }
    }
    return _ap->LocationChecks(locations);
}

bool Archipelago::LocationScouts(const json &jLocations, int sendAsHint)
{
    if (!_ap)
        return false; //throw std::runtime_error("Not connected");
    if (!_ap->allowSend())
        return false; //throw std::runtime_error("Tracker-only");
    if ((!jLocations.is_array() && !jLocations.is_object()))
        return false; //throw std::runtime_error("argument locations to LocationScouts must be array/table");

    std::list<std::int64_t> locations;
    for (auto& el: jLocations.items()) {
        if (el.value().is_number_integer()) {
            locations.push_back(el.value());
        } else {
            return false;
        }
    }
    return _ap->LocationScouts(locations, sendAsHint);
}

int Archipelago::Lua_Index(lua_State *L, const char* key) {
    if (strcmp(key, "PlayerNumber") == 0) {
        lua_pushinteger(L, _ap ? _ap->getPlayerNumber() : -1);
        return 1;
    }
    if (strcmp(key, "TeamNumber") == 0) {
        lua_pushinteger(L, _ap ? _ap->getTeamNumber() : -1);
        return 1;
    }
    if (strcmp(key, "CheckedLocations") == 0) {
        lua_newtable(L);
        if (_ap) {
            lua_Integer i=1;
            for (int64_t location : _ap->getCheckedLocations()) {
                lua_pushinteger(L, i++);
                if (std::numeric_limits<lua_Integer>::min() > location ||
                        std::numeric_limits<lua_Integer>::max() < location)
                    lua_pushnumber(L, location);
                else
                    lua_pushinteger(L, (lua_Integer)location);
                lua_settable(L, -3);
            }
        }
        return 1;
    }
    if (strcmp(key, "MissingLocations") == 0) {
        lua_newtable(L);
        if (_ap) {
            lua_Integer i=1;
            for (int64_t location : _ap->getMissingLocations()) {
                lua_pushinteger(L, i++);
                if (std::numeric_limits<lua_Integer>::min() > location ||
                        std::numeric_limits<lua_Integer>::max() < location)
                    lua_pushnumber(L, location);
                else
                    lua_pushinteger(L, (lua_Integer)location);
                lua_settable(L, -3);
            }
        }
        return 1;
    }
    printf("Get Archipelago.%s unknown\n", key);
    return 0;
}
