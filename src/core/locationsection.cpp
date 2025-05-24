#include "locationsection.h"
#include <lua.h>
#include <luaglue/luainterface.h>
#include "jsonutil.h"
#include "tracker.h"
#include "util.h"

using nlohmann::json;

Highlight HighlightFromString(const std::string& name)
{
    if (name == "avoid")
        return Highlight::AVOID;
    if (name == "no_priority")
        return Highlight::NO_PRIORITY;
    if (name == "unspecified")
        return Highlight::UNSPECIFIED;
    if (name == "priority")
        return Highlight::PRIORITY;
    return Highlight::NONE;
}

std::string_view HighlightToString(const Highlight highlight)
{
    switch (highlight) {
        case Highlight::AVOID:
            return "avoid";
        case Highlight::NONE:
            return "";
        case Highlight::NO_PRIORITY:
            return "no_priority";
        case Highlight::UNSPECIFIED:
            return "unspecified";
        case Highlight::PRIORITY:
            return "priority";
    }
    return ""; // invalid / unknown
}

const LuaInterface<LocationSection>::MethodMap LocationSection::Lua_Methods = {};

int LocationSection::Lua_Index(lua_State *L, const char *key)
{
    if (strcmp(key, "Owner") == 0) {
        lua_newtable(L); // dummy
        return 1;
    }
    if (strcmp(key, "AvailableChestCount") == 0) {
        lua_pushinteger(L, _itemCount - _itemCleared);
        return 1;
    }
    if (strcmp(key, "ChestCount") == 0) {
        lua_pushinteger(L, _itemCount);
        return 1;
    }
    if (strcmp(key, "FullID") == 0) {
        lua_pushstring(L, getFullID().c_str());
        return 1;
    }
    if (strcmp(key, "AccessibilityLevel") == 0) {
        lua_getglobal(L, "Tracker");
        Tracker* tracker = Tracker::luaL_testthis(L, -1);
        if (!tracker)
            return 0;
        if (_itemCleared >= _itemCount) {
            // if items are cleared, check if hosted items are collected
            bool done = true;
            for (auto& hosted: _hostedItems) {
                if (tracker->ProviderCountForCode(hosted) < 1) {
                    done = false;
                    break;
                }
            }
            if (done) {
                // if yes, cleared
                lua_pushinteger(L, (int)AccessibilityLevel::CLEARED);
                return 1;
            }
        }
        const auto res = static_cast<int>(tracker->isReachable(*this));
        lua_pushinteger(L, res);
        return 1;
    }
    if (strcmp(key, "Highlight") == 0) {
        lua_pushinteger(L, static_cast<int>(_highlight));
        return 1;
    }
    return 0;
}

bool LocationSection::Lua_NewIndex(lua_State *L, const char *key)
{
    if (strcmp(key,"AvailableChestCount")==0) {
        const int val = lua_isinteger(L, -1) ?
                static_cast<int>(lua_tointeger(L, -1)) :
                static_cast<int>(luaL_checknumber(L, -1));
        int cleared = _itemCount - val;
        if (cleared < 0)
            cleared = 0;
        if (cleared > _itemCount)
            cleared = _itemCleared;
        if (_itemCleared == cleared)
            return true;
        _itemCleared = cleared;
        onChange.emit(this);
        return true;
    }
    if (strcmp(key,"CapturedItem")==0) {
        // FIXME: implement this, see issue #12
        return true;
    }
    if (strcmp(key, "Highlight") == 0) {
        const auto val = lua_isinteger(L, -1) ?
                static_cast<Highlight>(lua_tointeger(L, -1)) :
                static_cast<Highlight>(luaL_checknumber(L, -1));
        if (val == _highlight)
            return true;
        _highlight = val;
        onChange.emit(this);
        return true;
    }
    return false;
}
