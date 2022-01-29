#include "location.h"
#include "jsonutil.h"
#include "util.h"
using nlohmann::json;

const LuaInterface<LocationSection>::MethodMap LocationSection::Lua_Methods = {};
int LocationSection::Lua_Index(lua_State *L, const char *key)
{
    if (strcmp(key,"Owner")==0) {
        lua_newtable(L); // dummy
        return 1;
    } else if (strcmp(key,"AvailableChestCount")==0) {
        lua_pushinteger(L, _itemCount - _itemCleared);
        return 1;
    } else if (strcmp(key,"ChestCount")==0) {
        lua_pushinteger(L, _itemCount);
        return 1;
    }
    return 0;
}
bool LocationSection::Lua_NewIndex(lua_State *L, const char *key)
{
    if (strcmp(key,"AvailableChestCount")==0) {
        int val = luaL_checkinteger(L, -1);
        int cleared = _itemCount - val;
        if (cleared < 0) cleared = 0;
        if (cleared > _itemCount) cleared = _itemCleared;
        if (_itemCleared == cleared) return true;
        _itemCleared = cleared;
        onChange.emit(this);
        return true;
    } else if (strcmp(key,"CapturedItem")==0) {
        // FIXME: implement this, see issue #12
        return true;
    }
    return false;
}

std::list<Location> Location::FromJSON(json& j, const std::list< std::list<std::string> >& parentAccessRules, const std::list< std::list<std::string> >& parentVisibilityRules, const std::string& parentName, const std::string& closedImgR, const std::string& openedImgR, const std::string& overlayBackgroundR)
{
    // TODO: pass inherited values as parent instead
    std::list<Location> locs;
    
    if (j.type() == json::value_t::array) {
        for (auto& v : j) {
            for (auto& loc : FromJSON(v, parentAccessRules, parentVisibilityRules, parentName, closedImgR, openedImgR, overlayBackgroundR)) {
                locs.push_back(std::move(loc)); // TODO: move constructor
            }
        }
        return locs;
    }
    if (j.type() != json::value_t::object) {
        fprintf(stderr, "Location: not an object\n");
        return locs;
    }
    
    std::string name = to_string(j["name"], "");
    std::list< std::list<std::string> > accessRules;
    if (j["access_rules"].type() == json::value_t::array) {
        // TODO: merge code with Section's access rules
        for (const auto& v : j["access_rules"]) {
            if (v.type() != json::value_t::string) {
                fprintf(stderr, "Location: bad access rule in \"%s\"\n",
                    sanitize_print(name).c_str());
                continue;
            }
            auto newRule = commasplit(v);
            for (auto oldRule : parentAccessRules) {
                for (auto& newTest : newRule) {
                    oldRule.push_back(newTest);
                }
                accessRules.push_back(oldRule);
            }
            if (parentAccessRules.empty()) {
                accessRules.push_back(newRule);
            }
        }
    } else {
        accessRules = parentAccessRules; // TODO: avoid copy
        if (j["access_rules"].type() != json::value_t::null) {
            fprintf(stderr, "Location: invalid access rules in \"%s\"\n",
                    sanitize_print(name).c_str());
        }
    }
    std::list< std::list<std::string> > visibilityRules;
    if (j["visibility_rules"].type() == json::value_t::array) {
        // TODO: merge code with Section's access rules
        for (const auto& v : j["visibility_rules"]) {
            if (v.type() != json::value_t::string) {
                fprintf(stderr, "Location: bad visibility rule in \"%s\"\n",
                    sanitize_print(name).c_str());
                continue;
            }
            auto newRule = commasplit(v);
            for (auto oldRule : parentVisibilityRules) {
                for (auto& newTest : newRule) {
                    oldRule.push_back(newTest);
                }
                visibilityRules.push_back(oldRule);
            }
            if (parentVisibilityRules.empty()) {
                visibilityRules.push_back(newRule);
            }
        }
    } else {
        visibilityRules = parentVisibilityRules; // TODO: avoid copy
        if (j["visibility_rules"].type() != json::value_t::null) {
            fprintf(stderr, "Location: invalid visibility rules in \"%s\"\n",
                    sanitize_print(name).c_str());
        }
    }
    
    std::string closedImg = to_string(j["chest_unopened_img"], closedImgR); // TODO: avoid copy
    std::string openedImg = to_string(j["chest_opened_img"], openedImgR);
    std::string overlayBackground = to_string(j["overlay_background"], overlayBackgroundR);

    // TODO: if maplocation or sections in j, add locations to locs
    if (j["sections"].type() == json::value_t::array)
    {
        Location loc;
        loc._name = name;
        loc._parentName = parentName;
        loc._id = loc._parentName.empty() ? loc._name : (loc._parentName + "/" + loc._name);
        if (j["map_locations"].type() == json::value_t::array) {
            for (auto& v : j["map_locations"]) {
                if (v.type() != json::value_t::object) {
                    fprintf(stderr, "Location: bad map location\n");
                    continue;
                }
                loc._mapLocations.push_back(MapLocation::FromJSON(v));
            }
        } else if (j["map_locations"].type() != json::value_t::null) {
            fprintf(stderr, "Location: invalid map locations\n");
        }
        for (auto& v: j["sections"]) {
            if (v.type() != json::value_t::object) {
                fprintf(stderr, "Location: bad section\n");
                continue;
            }
            loc._sections.push_back(LocationSection::FromJSON(v, accessRules, visibilityRules, closedImg, openedImg, overlayBackground));
        }
        locs.push_back(loc);
    }
    else if (j["map_locations"].type() != json::value_t::null &&
             j["sections"].type() == json::value_t::null)
    {
        fprintf(stderr, "Location: incomplete object\n");
    }
    
    if (j["children"].type() == json::value_t::array) {
        std::string fullname = parentName.empty() ? name : (parentName + "/" + name);
        for (auto& loc : Location::FromJSON(j["children"], accessRules, visibilityRules, fullname, closedImg, openedImg, overlayBackground)) {
            locs.push_back(std::move(loc));
        }
    } else if (j["children"].type() != json::value_t::null) {
        fprintf(stderr, "Location: bad children\n");
    }
    
    return locs;
}


Location::MapLocation Location::MapLocation::FromJSON(json& j)
{
    MapLocation maploc;
    maploc._mapName = to_string(j["map"],"");
    maploc._x = to_int(j["x"],0);
    maploc._y = to_int(j["y"],0);    
    return maploc;
}


LocationSection LocationSection::FromJSON(json& j, const std::list< std::list<std::string> >& parentAccessRules, const std::list< std::list<std::string> >& parentVisibilityRules, const std::string& closedImg, const std::string& openedImg, const std::string& overlayBackground)
{
    // TODO: pass inherited values as parent instead
    LocationSection sec;
    sec._name = to_string(j["name"],sec._name);
    sec._clearAsGroup = to_bool(j["clear_as_group"],sec._clearAsGroup);
    sec._closedImg = to_string(j["chest_unopened_img"], closedImg);
    sec._openedImg = to_string(j["chest_opened_img"], openedImg);
    sec._overlayBackground = to_string(j["overlay_background"], overlayBackground);
    sec._itemCount = to_int(j["item_count"], sec._itemCount);
    auto tmp = to_string(j["hosted_item"], "");
    commasplit(tmp, sec._hostedItems);
    if (j["access_rules"].type() == json::value_t::array) {
        // TODO: merge code with Location's access rules
        for (const auto& v : j["access_rules"]) {
            if (v.type() != json::value_t::string) {
                fprintf(stderr, "Location: bad access rule in \"%s\"\n",
                    sanitize_print(sec._name).c_str());
                continue;
            }
            auto newRule = commasplit(v);
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
    } else {
        sec._accessRules = parentAccessRules;
        if (j["access_rules"].type() != json::value_t::null) {
            fprintf(stderr, "Location: Section: invalid access rules in \"%s\"\n",
                    sanitize_print(sec._name).c_str());
        }
    }
    if (j["visibility_rules"].type() == json::value_t::array) {
        // TODO: merge code with Location's access rules
        for (const auto& v : j["visibility_rules"]) {
            if (v.type() != json::value_t::string) {
                fprintf(stderr, "Location: bad visibility rule in \"%s\"\n",
                    sanitize_print(sec._name).c_str());
                continue;
            }
            auto newRule = commasplit(v);
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
    } else {
        sec._visibilityRules = parentVisibilityRules;
        if (j["visibility_rules"].type() != json::value_t::null) {
            fprintf(stderr, "Location: Section: invalid visibility rules in \"%s\"\n",
                    sanitize_print(sec._name).c_str());
        }
    }
    return sec;
}


bool LocationSection::clearItem()
{
    if (_itemCleared >= _itemCount) return false;
    if (_clearAsGroup)
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
    return {
        {"cleared", _itemCleared}
    };
}
bool LocationSection::load(json& j)
{
    if (j.type() == json::value_t::object) {
        int val = to_int(j["cleared"], _itemCleared);
        if (val != _itemCleared) {
            _itemCleared = val;
            onChange.emit(this);
        }
        return true;
    }
    return false;
}


#ifndef NDEBUG
#include <stdio.h>
void Location::dump(bool compact)
{
    if (compact) {
        for (const auto& sec: _sections)
            printf("@%s/%s: ...\n", _id.c_str(), sec.getName().c_str());
        return;
    }
    
    printf("%s%s%s = {\n  map_locations = [ ", 
        _parentName.empty()?"":_parentName.c_str(),
        _parentName.empty()?"":" -> ",
        _name.c_str());
    bool firstMapLocation = true;
    for (auto& mapLocation : _mapLocations) {
        if (!firstMapLocation) printf(", ");
        firstMapLocation = false;
        printf("{ [%d,%d] in \"%s\" }", mapLocation.getX(), mapLocation.getY(), mapLocation.getMap().c_str());
    }
    printf(" ],\n  sections = [\n");
    bool firstSection = true;
    for (const auto& section : _sections) {
        if (!firstSection) printf(",\n");
        firstSection = false;
        printf("    { \"%s\", %d items + %d hosted, rules = [\n", 
                section.getName().c_str(),
                section.getItemCount(), (int)section.getHostedItems().size());
        bool firstRule = true;
        for (const auto& rule : section.getAccessRules()) {
            printf("   %s (", firstRule ? "  " : "OR");
            firstRule = false;
            bool firstTest = true;
            for (const auto& test : rule) {
                if (!firstTest) printf(" AND ");
                firstTest = false;
                printf("%s", test.c_str());
            }
            printf(")\n");
        }
        printf("    ] }");
    }
    printf("\n");
    printf("  ]\n}\n");
}
#endif
