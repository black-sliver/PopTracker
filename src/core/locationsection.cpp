#include "locationsection.h"
#include <lua.h>
#include <luaglue/luainterface.h>
#include "jsonutil.h"
#include "rule.h"
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

LocationSection LocationSection::FromJSON(
    json& j,
    const std::string& parentId,
    const bool glitchedScoutableAsGlitched,
    const std::list< std::list<std::string> >& parentAccessRules,
    const std::list< std::list<std::string> >& parentVisibilityRules,
    const std::string& closedImg,
    const std::string& openedImg,
    const std::string& overlayBackground)
{
    // TODO: pass inherited values as parent instead
    LocationSection sec;
    sec._parentId = parentId;
    sec._name = to_string(j["name"],sec._name);
    sec._clearAsGroup = to_bool(j["clear_as_group"],sec._clearAsGroup);
    sec._closedImg = to_string(j["chest_unopened_img"], closedImg);
    sec._openedImg = to_string(j["chest_opened_img"], openedImg);
    sec._overlayBackground = to_string(j["overlay_background"], overlayBackground);
    auto tmp = to_string(j["hosted_item"], "");
    commasplit(tmp, sec._hostedItems);
    sec._ref = to_string(j, "ref", "");
    sec._itemCount = sec._hostedItems.empty() && sec._ref.empty() ? 1 : 0;
    sec._itemCount = to_int(j["item_count"], sec._itemCount);
    bool nonEmpty = sec._itemCount > 0 || !sec._hostedItems.empty();
    sec._itemSize = LayoutNode::to_pixel(LayoutNode::to_size(j["item_size"],{-1,-1}));
    sec._itemSize.x = to_int(j["item_width"], sec._itemSize.x);
    sec._itemSize.y = to_int(j["item_height"], sec._itemSize.y);
    sec._glitchedScoutableAsGlitched = to_bool(j["inspectable_sequence_break"], glitchedScoutableAsGlitched);

    if (j["access_rules"].is_array() && !j["access_rules"].empty()) {
        // TODO: merge code with Location's access rules
        nonEmpty = true;
        for (const auto& v : j["access_rules"]) {
            std::list<std::string> newRule;
            if (!parseRule(v, newRule, "LocationSection", "access", sec._name))
                continue;
            for (auto oldRule : parentAccessRules) {
                for (auto& newTest : newRule) {
                    oldRule.push_back(newTest);
                }
                sec._accessRules.push_back(oldRule);
            }
            if (parentAccessRules.empty()) {
                sec._accessRules.push_back(newRule);
            }
        }
    } else if (j["access_rules"].is_string() && !j["access_rules"].empty()) {
        // single string access rule, same as [["code"]]
        const std::string& newTest = j["access_rules"];
        for (auto oldRule : parentAccessRules) {
            oldRule.push_back(newTest);
            sec._accessRules.push_back(oldRule);
        }
        if (parentAccessRules.empty()) {
            sec._accessRules.push_back({newTest});
        }
    } else {
        sec._accessRules = parentAccessRules;
        if (!j["access_rules"].is_null() && !j["access_rules"].is_array()) {
            fprintf(stderr, "Location: Section: invalid access rules in \"%s\"\n",
                    sanitize_print(sec._name).c_str());
        }
    }
    if (j["visibility_rules"].is_array() && !j["visibility_rules"].empty()) {
        // TODO: merge code with Location's access rules
        nonEmpty = true;
        for (const auto& v : j["visibility_rules"]) {
            std::list<std::string> newRule;
            if (!parseRule(v, newRule, "LocationSection", "visibility", sec._name))
                continue;
            for (auto oldRule : parentVisibilityRules) {
                for (auto& newTest : newRule) {
                    oldRule.push_back(newTest);
                }
                sec._visibilityRules.push_back(oldRule);
            }
            if (parentVisibilityRules.empty()) {
                sec._visibilityRules.push_back(newRule);
            }
        }
    } else if (j["visibility_rules"].is_string() && !j["visibility_rules"].empty()) {
        // single string visibility rule, same as [["code"]]
        const std::string& newTest = j["visibility_rules"];
        for (auto oldRule : parentVisibilityRules) {
            oldRule.push_back(newTest);
            sec._visibilityRules.push_back(oldRule);
        }
        if (parentVisibilityRules.empty()) {
            sec._visibilityRules.push_back({newTest});
        }
    } else {
        sec._visibilityRules = parentVisibilityRules;
        if (!j["visibility_rules"].is_null() && !j["visibility_rules"].is_array()) {
            fprintf(stderr, "Location: Section: invalid visibility rules in \"%s\"\n",
                    sanitize_print(sec._name).c_str());
        }
    }

    if (!sec._ref.empty() && nonEmpty) {
        fprintf(stderr, "Location: Section: extra data in section \"%s\" with \"ref\"\n",
                sanitize_print(sec._name).c_str());
    }

    return sec;
}


bool LocationSection::clearItem(bool all)
{
    if (_itemCleared >= _itemCount) return false;
    if (_clearAsGroup || all)
        _itemCleared = _itemCount;
    else
        _itemCleared++;
    onChange.emit(this);
    return true;
}

bool LocationSection::unclearItem()
{
    if (_itemCleared == 0) return false;
    if (_clearAsGroup)
        _itemCleared = 0;
    else
        _itemCleared--;
    onChange.emit(this);
    return true;
}

json LocationSection::save() const
{
    json j = json::object();
    if (_itemCleared)
        j["cleared"] = _itemCleared;
    if (_highlight != Highlight::NONE)
        j["highlight"] = HighlightToString(_highlight);
    return j;
}

bool LocationSection::load(json& j)
{
    if (j.type() == json::value_t::object) {
        bool changed = false;
        const int val = j.value("cleared", 0);
        if (val != _itemCleared) {
            _itemCleared = val;
            changed = true;
        }
        const auto highlight = HighlightFromString(j.value("highlight", ""));
        if (highlight != _highlight) {
            _highlight = highlight;
            changed = true;
        }
        if (changed)
            onChange.emit(this);
        return true;
    }
    return false;
}

bool LocationSection::operator<(const LocationSection& rhs) const
{
    if (this->getParentID() == rhs.getParentID())
        return this->getName() < rhs.getName();
    return this->getParentID() < rhs.getParentID();
}

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
