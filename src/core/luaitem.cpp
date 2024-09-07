#include "luaitem.h"
#include <luaglue/luamethod.h>
#include <luaglue/lua_json.h>
#include "jsonutil.h"
#include "util.h"
using nlohmann::json;


#define DEBUG_printf(...)
//#define DEBUG_printf printf


const LuaInterface<LuaItem>::MethodMap LuaItem::Lua_Methods = {
    LUA_METHOD(LuaItem, Get, const char*),
    LUA_METHOD(LuaItem, Set, const char*, LuaVariant),
    LUA_METHOD(LuaItem, SetOverlay, const char*),
    LUA_METHOD(LuaItem, SetOverlayBackground, const char*),
    LUA_METHOD(LuaItem, SetOverlayAlign, const char*),
    LUA_METHOD(LuaItem, SetOverlayFontSize, int),
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
        lua_pushstring(L, _fullImg.c_str());
        return 1;
    } else if (strcmp(key,"IconMods")==0) {
        std::string imgMods;
        for (auto& mod: _extraImgMods) {
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
    } else if (strcmp(key,"OnMiddleClickFunc") == 0) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, _onMiddleClickFunc.ref);
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
    } else if (strcmp(key,"Owner")==0) {
        lua_newtable(L); // dummy
        return 1;
    } else if (strcmp(key, "Type") == 0) {
        lua_pushstring(L, Type2Str(_type).c_str());
        return 1;
    }
    
    return 0;
}

bool LuaItem::Lua_NewIndex(lua_State *L, const char *key) {
    // TODO: lua_unref old references when assigning for callbacks
    _L = L;
    if (strcmp(key,"Name")==0) {
        std::string s = luaL_checkstring(L, -1);
        if (_name != s) {
            _name = s;
            onChange.emit(this);
        }
        return true;
    } else if (strcmp(key,"ItemState")==0) {
        if (_itemState.valid()) luaL_unref(L, LUA_REGISTRYINDEX, _itemState.ref);
        _itemState.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"Icon")==0) {
        // NOTE: this is a fake ImageReference, not just a path, so this can include ":mods" to preapply mods
        if (lua_type(L, -1) == LUA_TNIL) {
            if (!_fullImg.empty()) {
                _fullImg.clear();
                _img.clear();
                onChange.emit(this);
            }
            return true;
        }
        std::string s = luaL_checkstring(L,-1);
        if (_fullImg != s) {
            _fullImg = s;
            parseFullImg(); // turn fake ImageReference into _img and _mods
            onChange.emit(this);
        }
        return true;
    } else if (strcmp(key,"IconMods")==0) {
        // NOTE: these are applied on top of .Icon ImageReference
        std::string s = luaL_checkstring(L,-1);
        auto mods = commasplit<std::list>(s);
        if (_extraImgMods != mods) {
            _extraImgMods = mods;
            parseFullImg();
            onChange.emit(this);
        }
        return true;
    } else if (strcmp(key,"OnLeftClickFunc")==0) {
        _onLeftClickFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"OnRightClickFunc")==0) {
        _onRightClickFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
        return true;
    } else if (strcmp(key,"OnMiddleClickFunc") == 0) {
        _onMiddleClickFunc.ref = luaL_ref(L, LUA_REGISTRYINDEX); // pop copy and store
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
        lua_pop(_L, 1);
        return false;
    }
    bool res = lua_toboolean(_L, -1);
    lua_pop(_L,1);
    return res;
}

int LuaItem::providesCode(const std::string& code) const
{
    if (!_providesCodeFunc.valid()) return false;
    lua_rawgeti(_L, LUA_REGISTRYINDEX, _providesCodeFunc.ref);
    Lua_Push(_L); // arg1: this
    lua_pushstring(_L, code.c_str()); // arg2: code
    if (lua_pcall(_L, 2, 1, 0)) {
        printf("Error calling Item:ProvidesCode: %s\n", lua_tostring(_L, -1));
        lua_pop(_L, 1);
        return false;
    }

    int res;

    if (lua_isinteger(_L, -1)) {
        res = (int)lua_tointeger(_L, -1);
    } else if (lua_isboolean(_L, -1)) {
        res = lua_toboolean(_L, -1) ? 1 : 0;
    } else if (lua_isnumber(_L, -1)) {
        res = (int)lua_tonumber(_L, -1);
    } else {
        res = 0;
        printf("Item:ProvidesCode returned unexpected type %s for %s\n",
                lua_typename(_L, lua_type(_L, -1)), sanitize_print(_name).c_str());
    }
    lua_pop(_L,1);

    return res;
}

bool LuaItem::changeState(Action action)
{
    // for now we'll just have Left=Next send left-click, Right=Prev send right-click and Toggle send middle-click
    if (action == BaseItem::Action::Secondary || action == BaseItem::Action::Prev)
    {
        if (!_onRightClickFunc.valid())
            return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _onRightClickFunc.ref);
        Lua_Push(_L); // arg1: this
        if (lua_pcall(_L, 1, 0, 0)) {
            printf("Error calling Item:onRightClick: %s\n", lua_tostring(_L, -1));
            lua_pop(_L, 1);
            return false;
        }
    }
    else if (action == BaseItem::Action::Toggle)
    {
        if (!_onMiddleClickFunc.valid())
            return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _onMiddleClickFunc.ref);
        Lua_Push(_L); // arg1: this
        if (lua_pcall(_L, 1, 0, 0)) {
            printf("Error calling Item:onMiddleClick: %s\n", lua_tostring(_L, -1));
            lua_pop(_L, 1);
            return false;
        }
    }
    else
    {
        if (!_onLeftClickFunc.valid())
            return false;
        lua_rawgeti(_L, LUA_REGISTRYINDEX, _onLeftClickFunc.ref);
        Lua_Push(_L); // arg1: this
        if (lua_pcall(_L, 1, 0, 0)) {
            printf("Error calling Item:onLeftClick: %s\n", lua_tostring(_L, -1));
            lua_pop(_L, 1);
            return false;
        }
    }

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
        printf("Error calling Item:propertyChanged for \"%s\": %s\n",
                sanitize_print(_name).c_str(), lua_tostring(_L, -1));
        lua_pop(_L, 1);
        return;
    }
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
        lua_pop(_L, 1);
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
        printf("Error calling Item:load for \"%s\": %s\n",
                sanitize_print(_name).c_str(), lua_tostring(_L, -1));
        lua_pop(_L, 1);
        return false;
    }
    return true;
}

void LuaItem::parseFullImg()
{
    auto pos = _fullImg.find(':');
    if (pos == _fullImg.npos) {
        _img = _fullImg;
        _imgMods = _extraImgMods;
    } else {
        _img = _fullImg.substr(0, pos);
        _imgMods = commasplit<std::list>(_fullImg.substr(pos + 1));
        _imgMods.insert(_imgMods.end(), _extraImgMods.begin(), _extraImgMods.end());
    }
}
