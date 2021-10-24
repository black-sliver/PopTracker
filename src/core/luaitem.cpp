#include "luaitem.h"
#include "../luaglue/luamethod.h"
#include "../luaglue/lua_json.h"
#include "jsonutil.h"
using nlohmann::json;


#define DEBUG_printf(...)
//#define DEBUG_printf printf


const LuaInterface<LuaItem>::MethodMap LuaItem::Lua_Methods = {
    LUA_METHOD(LuaItem, Get, const char*),
    LUA_METHOD(LuaItem, Set, const char*, LuaVariant),
    LUA_METHOD(LuaItem, SetOverlay, const char*),
    LUA_METHOD(LuaItem, SetOverlayBackground, const char*),
};

int LuaItem::Lua_Index(lua_State *L, const char* key) {
    _L = L;
    if (strcmp(key,"Name")==0) {
        lua_pushstring(L, _name.c_str());
        return 1;
    } else if (strcmp(key,"ItemState")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _itemState.ref);
        return 1;
    } else if (strcmp(key,"Icon")==0) {
        lua_pushstring(L, _img.c_str());
        return 1;
    } else if (strcmp(key,"IconMods")==0) {
        std::string imgMods;
        for (auto& mod: _imgMods) {
            if (!imgMods.empty()) imgMods += ", ";
            imgMods += mod;
        }
        lua_pushstring(L, imgMods.c_str());
        return 1;
    } else if (strcmp(key,"OnLeftClickFunc")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _onLeftClickFunc.ref);
        return 1;
    } else if (strcmp(key,"OnRightClickFunc")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _onRightClickFunc.ref);
        return 1;
    } else if (strcmp(key,"CanProvideCodeFunc")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _canProvideCodeFunc.ref);
        return 1;
    } else if (strcmp(key,"ProvidesCodeFunc")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _providesCodeFunc.ref);
        return 1;
    } else if (strcmp(key,"AdvanceToCodeFunc")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _advanceToCodeFunc.ref);
        return 1;
    } else if (strcmp(key,"SaveFunc")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _saveFunc.ref);
        return 1;
    } else if (strcmp(key,"LoadFunc")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _loadFunc.ref);
        return 1;
    } else if (strcmp(key,"PropertyChangedFunc")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _propertyChangedFunc.ref);
        return 1;
    } else if (strcmp(key,"PotentialIcon")==0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _potentialIcon.ref);
        return 1;
    } else if (strcmp(key,"BadgeText")==0) {
        lua_pushstring(L, "");
        return 1; // FIXME: not implemented
    } else if (strcmp(key,"BadgeTextColor")==0) {
        lua_pushstring(L, "");
        return 1; // FIXME: not implemented
    } else if (strcmp(key,"MaskInput")==0) {
        lua_pushstring(L, "");
        return 1; // FIXME: not implemented
    }
    
    return 0;
}

bool LuaItem::Lua_NewIndex(lua_State *L, const char *key) {
    // TODO: lua_unref old references when assigning for callbacks
    _L = L;
    if (strcmp(key,"Name")==0) {
        _name = luaL_checkstring(L, -1);
        return true;
    } else if (strcmp(key,"ItemState")==0) {
        if (_itemState.valid()) luaL_unref(L, LUA_REGISTRYINDEX, _itemState.ref);
        _itemState.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"Icon")==0) {
        std::string s = luaL_checkstring(L,-1);
        if (_img != s) {
            _img = s;
            onChange.emit(this);
        }
        return true;
    } else if (strcmp(key,"IconMods")==0) {
        std::string s = luaL_checkstring(L,-1);
        auto mods = commasplit(s);
        if (_imgMods != mods) {
            _imgMods = mods;
            onChange.emit(this);
        }
        return true;
    } else if (strcmp(key,"OnLeftClickFunc")==0) {
        _onLeftClickFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"OnRightClickFunc")==0) {
        _onRightClickFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"CanProvideCodeFunc")==0) {
        _canProvideCodeFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"ProvidesCodeFunc")==0) {
        _providesCodeFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"AdvanceToCodeFunc")==0) {
        _advanceToCodeFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"SaveFunc")==0) {
        _saveFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"LoadFunc")==0) {
        _loadFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"PropertyChangedFunc")==0) {
        _propertyChangedFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"PotentialIcon")==0) {
        if (_potentialIcon.valid()) luaL_unref(L, LUA_REGISTRYINDEX, _potentialIcon.ref);
        _potentialIcon.ref = luaL_ref(L, LUA_REGISTRYINDEX);
        return true;
    } else if (strcmp(key,"BadgeText")==0) {
        return true; // FIXME: not implemented
    } else if (strcmp(key,"BadgeTextColor")==0) {
        return true; // FIXME: not implemented
    } else if (strcmp(key,"MaskInput")==0) {
        return true; // FIXME: not implemented
    }
    
    return false;
}


bool LuaItem::canProvideCode(const std::string& code) const
{
    if (!_canProvideCodeFunc.valid()) return false;
    lua_rawgeti(_L, LUA_REGISTRYINDEX, _canProvideCodeFunc.ref);
    Lua_Push(_L); // arg1: this
    lua_pushstring(_L, code.c_str()); // arg2: code
    if (lua_pcall(_L, 2, 1, 0)) {
        printf("Error calling Item:CanProvideCode: %s\n", lua_tostring(_L, -1));
        // TODO: clean up lua stack
        return false;
    }
    bool res = lua_toboolean(_L, -1);
    lua_pop(_L,1);
    return res;
}
int LuaItem::providesCode(const std::string code) const
{
    if (!_providesCodeFunc.valid()) return false;
    lua_rawgeti(_L, LUA_REGISTRYINDEX, _providesCodeFunc.ref);
    Lua_Push(_L); // arg1: this
    lua_pushstring(_L, code.c_str()); // arg2: code
    if (lua_pcall(_L, 2, 1, 0)) {
        printf("Error calling Item:ProvidesCode: %s\n", lua_tostring(_L, -1));
        // TODO: clean up lua stack
        return false;
    }
    
    int res = luaL_checkinteger(_L, -1);
    lua_pop(_L,1);
    return res;
}
bool LuaItem::changeState(Action action)
{
    // for now we'll just have Left=Next send left-click, Right=Prev send right-click
    if (action == BaseItem::Action::Secondary || action == BaseItem::Action::Prev)
    {
        if (!_onRightClickFunc.valid()) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _onRightClickFunc.ref);
        Lua_Push(_L); // arg1: this
        if (lua_pcall(_L, 1, 0, 0)) {
            printf("Error calling Item:onRightClick: %s\n", lua_tostring(_L, -1));
            // TODO: clean up lua stack
            return false;
        }
    }
    else
    {
        if (!_onLeftClickFunc.valid()) return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _onLeftClickFunc.ref);
        Lua_Push(_L); // arg1: this
        if (lua_pcall(_L, 1, 0, 0)) {
            printf("Error calling Item:onLeftClick: %s\n", lua_tostring(_L, -1));
            // TODO: clean up lua stack
            return false;
        }
    }
    // TODO: clean up lua stack
    return true;
}


LuaVariant LuaItem::Get(const char* s)
{
    if (_itemState.valid()) { // grab property from .itemState
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _itemState.ref);
        lua_getfield(_L, -1, s);
        LuaVariant v;
        v.Lua_Get(_L, -1);
        DEBUG_printf("LuaItem(\"%s\"):Get(\"%s\") = %s\n", _name.c_str(), s, v.toString().c_str());
        // TODO: we probably need to pop from lua stack
        return v;
    } else { // grab property from internal map
        const auto& it = _properties.find(s);
        if (it == _properties.end()) {
            DEBUG_printf("LuaItem(\"%s\"):Get(\"%s\") = none\n", _name.c_str(), s);
            return {};
        }
        DEBUG_printf("LuaItem(\"%s\"):Get(\"%s\") = %s\n", _name.c_str(), s, it->second.toString().c_str());
        return it->second;
    }
}
void LuaItem::Set(const char* s, LuaVariant v)
{
    DEBUG_printf("LuaItem(\"%s\"):Set(\"%s\",%s)\n  ", _name.c_str(), s, v.toString().c_str());
    
    if (Get(s) == v) return;
    if (_itemState.valid()) {
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _itemState.ref);
        v.Lua_Push(_L);
        lua_setfield(_L, -2, s);
        DEBUG_printf("itemState[%s] = %s\n", s, v.toString().c_str());
        // TODO: we probably need to pop from lua stack
    } else {
        _properties[s] = v;
    }
    if (!_propertyChangedFunc.valid()) return;
    lua_rawgeti(_L, LUA_REGISTRYINDEX, _propertyChangedFunc.ref);
    Lua_Push(_L);          // arg1: this
    lua_pushstring(_L, s); // arg2: key
    v.Lua_Push(_L);        // arg3: value
    if (lua_pcall(_L, 3, 0, 0)) {
        printf("Error calling Item:propertyChanged: %s\n", lua_tostring(_L, -1));
        // TODO: clean up lua stack
        return;
    }
    // TODO: clean up lua stack
    DEBUG_printf("LuaItem(\"%s\"):propertyChanged(\"%s\",%s) called\n", _name.c_str(), s, v.toString().c_str());
}

json LuaItem::save() const
{
    json j;
    
    if (!_saveFunc.valid()) return j;
    lua_rawgeti(_L, LUA_REGISTRYINDEX, _saveFunc.ref);
    Lua_Push(_L); // arg1: this
    if (lua_pcall(_L, 1, 1, 0)) {
        printf("Error calling Item:save: %s\n", lua_tostring(_L, -1));
        // TODO: clean up lua stack
        return j;
    }
    j = lua_to_json(_L);
    lua_pop(_L,1);
    return j;
}
bool LuaItem::load(json& j)
{
    if (j.type() == json::value_t::null) return true;
    if (!_loadFunc.valid()) return false;
    
    lua_rawgeti(_L, LUA_REGISTRYINDEX, _loadFunc.ref);
    Lua_Push(_L); // arg1: this
    json_to_lua(_L, j); // arg2: data table
    if (lua_pcall(_L, 2, 0, 0)) {
        printf("Error calling Item:load: %s\n", lua_tostring(_L, -1));
        // TODO: clean up lua stack
        return false;
    }
    return true;
}