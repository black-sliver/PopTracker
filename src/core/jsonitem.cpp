#include "jsonitem.h"
#include "jsonutil.h"
#include <luaglue/luamethod.h>
using nlohmann::json;

const LuaInterface<JsonItem>::MethodMap JsonItem::Lua_Methods = {
    LUA_METHOD(JsonItem, SetOverlay, const char*),
    LUA_METHOD(JsonItem, SetOverlayBackground, const char*),
    LUA_METHOD(JsonItem, SetOverlayAlign, const char*),
    LUA_METHOD(JsonItem, SetOverlayFontSize, int),
};

std::string JsonItem::getCodesString() const {
    // this returns a human-readable list, for debugging
    std::string s;
    for (const auto& code: _codes) {
        if (!s.empty()) s += ", ";
        s += code;
    }
    for (const auto& stage: _stages) {
        if (!s.empty()) s += ", ";
        s += stage.getCodesString();
    }
    return s;
}

const std::vector<std::string>& JsonItem::getCodes(int stage) const {
    if (_type == Type::TOGGLE) return _codes;
    if (stage>=0 && (size_t)stage<_stages.size()) return _stages[stage].getCodes();
    return _codes;
}

JsonItem JsonItem::FromJSONString(const std::string& j)
{
    return FromJSON(json::parse(j, nullptr, true, true));
}

JsonItem JsonItem::FromJSON(nlohmann::json&& j)
{
    auto tmp = j;
    return FromJSON(tmp);
}

JsonItem JsonItem::FromJSON(json& j)
{
    JsonItem item;
    
    item._name          = to_string(j["name"], "");
    item._type          = Str2Type(to_string(j["type"], ""));

    item._capturable    = to_bool(j["capturable"], false);
    item._loop          = to_bool(j["loop"], false);
    item._allowDisabled = to_bool(j["allow_disabled"], true);
    item._img           = to_string(j["img"], "");
    item._disabledImg   = to_string(j["disabled_img"], item._img);
    item._minCount      = to_int(j["min_quantity"], 0);
    item._maxCount      = to_int(j["max_quantity"], -1);
    item._increment     = to_int(j["increment"], 1);
    item._decrement     = to_int(j["decrement"], item._increment);
    item._overlayBackground = to_string(j["overlay_background"], "");
    item._overlayAlign = to_string(j["overlay_align"], "");
    item._overlayFontSize = to_int(j["overlay_font_size"], to_int(j["badge_font_size"], 0));

    commasplit(to_string(j["codes"], ""), item._codes);
    commasplit(to_string(j["img_mods"], ""), item._imgMods);
    std::string defaultDisabledImgMods = to_string(j["img_mods"], "");
    if (defaultDisabledImgMods.empty())
        defaultDisabledImgMods = "@disabled";
    else
        defaultDisabledImgMods += ",@disabled";
    commasplit(to_string(j["disabled_img_mods"], defaultDisabledImgMods), item._disabledImgMods);

    if (item._type == Type::TOGGLE || item._type == Type::TOGGLE_BADGED) {
        item._allowDisabled = true; // always true
    } else if (item._type == Type::PROGRESSIVE_TOGGLE) {
        item._loop = true;
        item._allowDisabled = true;
    } else if (item._type == Type::STATIC) {
        item._allowDisabled = false; // always false
        item._stage1 = 1;
    }

    if (j["stages"].type() == json::value_t::array) {
        for (auto& v: j["stages"]) {
            item._stages.push_back(Stage::FromJSON(v));
        }
    }
    
    if (item._type == Type::COMPOSITE_TOGGLE && j["images"].type() == json::value_t::array) {
        // we convert composite toggle to a 4-staged item and only have special
        // code for mouse presses and for linking it to the "parts" in Tracker
        std::string leftCode = to_string(j["item_left"], "");
        std::string rightCode = to_string(j["item_right"], "");
        for (uint8_t n=0; n<4; n++) {
            for (auto& v: j["images"]) {
                if (to_bool(v["left"],false) == !!(n&1) && to_bool(v["right"],false) == !!(n&2)) {
                    v["codes"] = (n==3) ? leftCode+","+rightCode :
                                 (n==2) ? rightCode : leftCode;
                    v["inherit_codes"] = false;
                    item._stages.push_back(Stage::FromJSON(v));
                    break;
                }
            }
        }
        item._stage1 = 1;
    }

    if (item._type == Type::TOGGLE_BADGED && j["base_item"].is_string()) {
        item._baseItem = j["base_item"];
    }

    if (item._type == Type::TOGGLE || item._type == Type::TOGGLE_BADGED || item._type == Type::PROGRESSIVE_TOGGLE) {
        if (to_bool(j["initial_active_state"], false))
            item._stage1 = 1;
    }

    item._stage2 = std::max(0,std::min(to_int(j["initial_stage_idx"],0), (int)item._stages.size()-1));
    item._count  = std::max(item._minCount, std::min(to_int(j["initial_quantity"],0), item._maxCount));
    if (item._type == Type::CONSUMABLE && item._count > 0) item._stage1=1;

#ifdef JSONITEM_CI_QUIRK
    for (const auto& code : item._codes)
        item._ciAllCodes.emplace(toLower(code));
    for (const auto& stage : item._stages)
        for (const auto& code : stage.getCodes())
            item._ciAllCodes.emplace(toLower(code));
#endif

    return item;
}


JsonItem::Stage JsonItem::Stage::FromJSONString(const std::string& j)
{
    return FromJSON(json::parse(j, nullptr, true, true));
}

JsonItem::Stage JsonItem::Stage::FromJSON(nlohmann::json&& j)
{
    auto tmp = j;
    return FromJSON(tmp);
}

JsonItem::Stage JsonItem::Stage::FromJSON(json& j)
{
    Stage stage;
    
    stage._img          = to_string(j["img"], "");
    stage._disabledImg  = to_string(j["disabled_img"], stage._img);
    stage._inheritCodes = to_bool(j["inherit_codes"], true);
    stage._name         = to_string(j["name"], "");
    
    commasplit(to_string(j["codes"], ""), stage._codes);
    commasplit(to_string(j["secondary_codes"], ""), stage._secondaryCodes);
    commasplit(to_string(j["img_mods"], ""), stage._imgMods);
    commasplit(to_string(j["disabled_img_mods"], to_string(j["img_mods"], "")+",@disabled"), stage._disabledImgMods);
    
    return stage;
}

std::string JsonItem::Stage::getCodesString() const {
    // this returns a human-readable list, for debugging
    std::string s;
    for (const auto& code: _codes) {
        if (!s.empty()) s += ", ";
        s += code;
    }
    return s;
}


bool JsonItem::_changeStateImpl(BaseItem::Action action) {
    if (_type == Type::TOGGLE) {
        // left,fwd = on, right,back = off, middle = toggle
        if (action == Action::Primary || action == Action::Next) {
            if (_stage1 > 0)
                return false;
            _stage1 = 1;
        } else if (action == Action::Secondary || action == Action::Prev) {
            if (_stage1 < 1 || !_allowDisabled)
                return false;
            _stage1 = 0;
        } else if (_allowDisabled) { // middle mouse = toggle
            _stage1 = !_stage1;
        }
    } else if (_type == Type::PROGRESSIVE && !_allowDisabled) {
        // left,fwd = next, right,back = prev, middle = next+always loop
        int n = _stage2;
        if (action == Action::Primary || action == Action::Next) {
            n++;
            if (n>=(int)_stages.size()) {
                if (_loop) n = 0;
                else n--;
            }
        } else if (action == Action::Secondary || action == Action::Prev) {
            n--;
            if (n<0) {
                if (_loop) n = (int)_stages.size()-1;
                else n++;
            }
        } else {
            // single button control
            n++;
            if (n >= (int)_stages.size()) n = 0;
        }
        if (n == _stage2) return false;
        _stage2 = n;
    } else if (_type == Type::PROGRESSIVE/* && _allowDisabled*/) {
        // left,fwd = next, right,back = prev, middle = next+always loop
        // has virtual 0th stage as disabled fist stage
        int a = _stage1;
        int n = _stage2;
        if (action == Action::Primary || action == Action::Next) {
            if (!a) {
                a = 1;
            } else {
                n++;
                if (n>=(int)_stages.size()) {
                    if (_loop) {
                        n = 0;
                        a = 0;
                    }
                    else n--;
                }
            }
        } else if (action == Action::Secondary || action == Action::Prev) {
            if (a && n == 0) {
                a = 0;
            } else {
                n--;
                if (n<0) {
                    if (_loop) {
                        n = (int)_stages.size()-1;
                        a = 1;
                    }
                    else n++;
                }
            }
        } else {
            // single button control
            if (!a) a = 1;
            else n++;
            if (n >= (int)_stages.size()) {
                n = 0;
                a = 0;
            }
        }
        if (a == _stage1 && n == _stage2) return false;
        _stage1 = a;
        _stage2 = n;
    } else if (_type == Type::PROGRESSIVE_TOGGLE) {
        // left,middle = toggle, right,fwd = next, back = prev
        if (action == Action::Primary || action == Action::Toggle) {
            _stage1 = !_stage1;
        } else if (action == Action::Secondary || action == Action::Next) {
            int n = _stage2+1;
            if (n>=(int)_stages.size()) {
                if (_loop || action == Action::Secondary) n = 0; // always loop for progressive + allowDisable + right mouse button
                else n--;
            }
            if (n == _stage2) return false;
            _stage2 = n;
        } else if (action == Action::Prev) {
            int n = _stage2-1;
            if (n<0) {
                if (_loop) n = _stages.size()-1;
                else n++;
            }
            if (n == _stage2) return false;
            _stage2 = n;
        } else {
            // single button control
            if (!_stage1) {
                _stage1 = 1;
                _stage2 = 0;
            } else {
                _stage2++;
                if (_stage2>=(int)_stages.size()) {
                    _stage1 = 0;
                    _stage2 = 0;
                }
            }
        }
    } else if (_type == Type::CONSUMABLE) {
        // left,fwd = +1, right,back = -1
        if (action == Action::Primary || action == Action::Next) {
            int n = _count+_increment;
            if (_maxCount>=0 && n>_maxCount) n = _maxCount;
            if (n < _minCount) n = _minCount;
            if (n == _count) return false;
            _count = n;
            _stage1 = (n>0);
        } else if (action == Action::Secondary || action == Action::Prev) {
            int n = _count-_decrement;
            if (_maxCount>=0 && n>_maxCount) n = _maxCount;
            if (n < _minCount) n = _minCount;
            if (n == _count) return false;
            _count = n;
            _stage1 = (n>0);
        } else {
            // single button control
            int n = _count+1;
            if (_maxCount>=0 && n>_maxCount) n = _minCount;
            _count = n;
            _stage1 = (n>0);
        }
    } else if (_type == Type::COMPOSITE_TOGGLE) {
        unsigned n = (unsigned)_stage2;
        if (action == Action::Primary) {
            n ^= 1;
        } else if (action == Action::Secondary) {
            n ^= 2;
        }
        if (n >= _stages.size()) return false;
        _stage1 = 1;
        _stage2 = (int)n;
    } else if (_type == Type::TOGGLE_BADGED) {
        // only handle right-click here
        if (action == Action::Secondary && _allowDisabled)
            _stage1 = !_stage1;
        else
            return false;
    } else {
        // not implemented
        printf("Unimplemented item action for type=%s\n", Type2Str(_type).c_str());
        return false;
    }
    return true;
}

int JsonItem::Lua_Index(lua_State *L, const char* key) {
    if (strcmp(key, "AcquiredCount")==0) {
        lua_pushinteger(L, _count);
        return 1;
    } else if (strcmp(key, "Active")==0) {
        if (!_allowDisabled)
            lua_pushboolean(L, 1);  // always Active
        lua_pushboolean(L, _stage1);
        return 1;
    } else if (strcmp(key, "CurrentStage")==0) {
        if (_type == Type::PROGRESSIVE && _allowDisabled) {
            // progressive toggles w/ allow_disabled have an added 0-stage.
            // TODO: implement this properly some time
            lua_pushinteger(L, _stage1 ? _stage2 + 1 : 0);
        } else {
            lua_pushinteger(L, _stage2);
        }
        return 1;
    } else if (strcmp(key, "MinCount")==0) {
        lua_pushinteger(L, _minCount);
        return 1;
    } else if (strcmp(key, "MaxCount")==0) {
        lua_pushinteger(L, _maxCount);
        return 1;
    } else if (strcmp(key, "Owner")==0) {
        lua_newtable(L); // dummy
        return 1;
    } else if (strcmp(key, "Increment")==0) {
        lua_pushinteger(L, _increment);
        return 1;
    } else if (strcmp(key, "Decrement")==0) {
        lua_pushinteger(L, _decrement);
        return 1;
    } else if (strcmp(key, "Type") == 0) {
        lua_pushstring(L, Type2Str(_type).c_str());
        return 1;
    } else if (strcmp(key, "Icon") == 0) {
        if (_imgOverridden)
            lua_pushstring(L, _imgOverride.c_str());
        else {
            const auto& mods = _stage1 ? getImageMods(_stage2) : getDisabledImageMods(_stage2);
            if (mods.empty()) {
                lua_pushstring(L, (_stage1 ? getImage(_stage2) : getDisabledImage(_stage2)).c_str());
            } else {
                std::string s = _stage1 ? getImage(_stage2) : getDisabledImage(_stage2);
                bool first = true;
                for (const auto& mod: mods) {
                    s += (first ? ":" : ",") + mod;
                    first = false;
                }
                lua_pushstring(L, s.c_str());
            }
        }
        return 1;
    } else if (strcmp(key, "IgnoreUserInput") == 0) {
        lua_pushboolean(L, _ignoreUserInput);
        return 1;
    }
    printf("Get JsonItem(%s).%s unknown\n", _name.c_str(), key);
    return 0;
}

bool JsonItem::Lua_NewIndex(lua_State *L, const char *key) {
    if (strcmp(key, "AcquiredCount")==0) {
        int val = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : (int)luaL_checknumber(L, -1);
        if (_count != val) {
            _count = val;
            if (_type == Type::CONSUMABLE)
                _stage1 = (_count>0);
            onChange.emit(this);
        }
        return true;
    } else if (strcmp(key, "Active")==0) {
        bool val = lua_isinteger(L, -1) ? (lua_tointeger(L, -1)>0) : (bool)lua_toboolean(L, -1);
        if (_type == Type::PROGRESSIVE && _allowDisabled && !val)
            _stage2 = 0;
        else if (!_allowDisabled)
            val = true;  // always Active
        if (_stage1 != val) {
            _stage1 = val;
            if (_imgOverridden) {
                _imgOverridden = false;
                _imgOverride.clear();
                onChange.emit(this);
                onDisplayChange.emit(this);
            } else {
                onChange.emit(this);
            }
        }
        return true;
    } else if (strcmp(key, "CurrentStage")==0) {
        int val = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : (int)luaL_checknumber(L, -1);
        if (val<0) val=0;
        int en = _stage1;
        if (_type == Type::PROGRESSIVE && _allowDisabled) {
            // progressive toggles w/ allow_disabled have an added 0-stage.
            // TODO: implement this properly some time
            if (val == 0) {
                en = 0;
                val = 0;
            } else {
                en = 1;
                val--;
            }
        }
        if (_stages.size()>0 && (size_t)val>=_stages.size()) val=(size_t)_stages.size()-1;
        if (_stage2 != val || _stage1 != en) {
            _stage1 = en;
            _stage2 = val;
            if (_imgOverridden) {
                _imgOverridden = false;
                _imgOverride.clear();
                onChange.emit(this);
                onDisplayChange.emit(this);
            } else {
                onChange.emit(this);
            }
        }
        return true;
    } else if (strcmp(key, "MinCount")==0) {
        int val = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : (int)luaL_checknumber(L, -1);
        if (_minCount != val) {
            _minCount = val;
            _minCountChanged = true;
            if (_minCount>_count) {
                _count = _minCount;
                onChange.emit(this);
            }
        }
        return true;
    } else if (strcmp(key, "MaxCount")==0) {
        int val = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : (int)luaL_checknumber(L, -1);
        if (_maxCount != val) {
            _maxCount = val;
            _maxCountChanged = true;
            if (_maxCount<_count && _maxCount>=0) { // less than 0 is infinite
                _count = _maxCount;
                onChange.emit(this);
            }
        }
        return true;
    } else if (strcmp(key, "Increment")==0) {
        int val = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : (int)luaL_checknumber(L, -1);
        if (_increment == val) return true;
        _increment = val;
        _incrementChanged = true;
        return true;
    } else if (strcmp(key, "Decrement")==0) {
        int val = lua_isinteger(L, -1) ? (int)lua_tointeger(L, -1) : (int)luaL_checknumber(L, -1);
        if (_decrement == val) return true;
        _decrement = val;
        _decrementChanged = true;
        return true;
    } else if (strcmp(key, "Icon") == 0) {
        // NOTE: this is a fake ImageRef, but only path is implemented yet
        if (lua_type(L, -1) == LUA_TNIL) {
            if (!_imgOverridden || !_imgOverride.empty()) {
                _imgOverridden = true;
                _imgOverride.clear();
                onDisplayChange.emit(this);
            }
            return true;
        }
        std::string s = luaL_checkstring(L, -1);
        if (!_imgOverridden || _imgOverride != s) { // TODO: also compare to current non-overriden image
            _imgOverridden = true;
            _imgOverride = s;
            onDisplayChange.emit(this);
        }
        return true;
    } else if (strcmp(key, "IgnoreUserInput") == 0) {
        auto t = lua_type(L, -1);
        _ignoreUserInput = (t == LUA_TBOOLEAN && lua_toboolean(L, -1))
                        || (t == LUA_TINTEGER && lua_tointeger(L, -1) > 0);
        return true;
    }
    printf("Set JsonItem(%s).%s unknown\n", _name.c_str(), key);
    return false;
}

json JsonItem::save() const
{
    json data = {
        { "overlay", _overlay },
        { "state", { _stage1, _stage2 } },
        { "count", _count },
    };
    if (_minCountChanged)
        data["min_count"] = _minCount;
    if (_maxCountChanged)
        data["max_count"] = _maxCount;
    if (_overlayBackgroundChanged)
        data["overlay_background"] = _overlayBackground;
    if (_overlayAlignChanged)
        data["overlay_align"] = _overlayAlign;
    if (_overlayFontSizeChanged)
        data["overlay_font_size"] = _overlayFontSize;
    if (_incrementChanged)
        data["increment"] = _increment;
    if (_decrementChanged)
        data["decrement"] = _decrement;
    if (_imgOverridden)
        data["img"] = _imgOverride;
    if (_ignoreUserInput)
        data["ignore_user_input"] = true;
    return data;
}

bool JsonItem::load(json& j)
{
    if (j.type() == json::value_t::object) {
        std::string overlay = to_string(j["overlay"], _overlay);
        std::string overlayBackground = to_string(j["overlay_background"], _overlayBackground);
        std::string overlayAlign = to_string(j["overlay_align"], _overlayAlign);
        int overlayFontSize = to_int(j["overlay_font_size"], _overlayFontSize);
        int increment = to_int(j["increment"], _increment);
        int decrement = to_int(j["decrement"], _decrement);
        auto state = j["state"];
        int stage1 = _stage1, stage2 = _stage2;
        if (state.type() == json::value_t::array) {
            stage1 = to_int(state[0],stage1);
            stage2 = to_int(state[1],stage2);
        }
        int count = to_int(j["count"],_count);
        int minCount = to_int(j["min_count"], _minCount);
        int maxCount = to_int(j["max_count"], _maxCount);
        auto itImg = j.find("img");
        bool imgOverridden = itImg != j.end() && itImg->type() == json::value_t::string;
        std::string imgOverride = imgOverridden ? itImg->get<std::string>() : _imgOverride;
        bool ignoreUserInput = to_bool(j["ignore_user_input"], _ignoreUserInput);
        bool changed = false;
        bool displayChanged = false;
        if (_count != count || _minCount != minCount || _maxCount != maxCount
                || _stage1 != stage1 || _stage2 != stage2
                || _overlay != overlay
                || _overlayBackground != overlayBackground
                || _overlayAlign != overlayAlign
                || _overlayFontSize != overlayFontSize
                || _increment != increment
                || _decrement != decrement
                || _ignoreUserInput != ignoreUserInput) {
            _count = count;
            _minCountChanged = _minCount != minCount;
            _minCount = minCount;
            _maxCountChanged = _maxCount != maxCount;
            _maxCount = maxCount;
            _stage1 = stage1;
            _stage2 = stage2;
            _overlay = overlay;
            _overlayBackgroundChanged = _overlayBackground != overlayBackground;
            _overlayBackground = overlayBackground;
            _overlayAlignChanged = _overlayAlign != overlayAlign;
            _overlayAlign = overlayAlign;
            _overlayFontSizeChanged = _overlayFontSize != overlayFontSize;
            _overlayFontSize = overlayFontSize;
            _incrementChanged = _increment != increment;
            _increment = increment;
            _decrementChanged = _decrement != decrement;
            _decrement = decrement;
            _ignoreUserInput = ignoreUserInput;
            changed = true;
        }
        if (_imgOverridden != imgOverridden
                || _imgOverride != imgOverride) {
            _imgOverridden = imgOverridden;
            _imgOverride = imgOverride;
            displayChanged = true;
        }
        if (changed)
            onChange.emit(this);
        if (displayChanged)
            onDisplayChange.emit(this);
        return true;
    }
    return false;
}
