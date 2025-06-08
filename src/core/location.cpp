#include "location.h"
#include <lua.h>
#include <luaglue/luainterface.h>
#include "jsonutil.h"
#include "rule.h"
#include "tracker.h"
#include "util.h"


using nlohmann::json;


const LuaInterface<Location>::MethodMap Location::Lua_Methods = {};

int Location::Lua_Index(lua_State *L, const char *key)
{
    if (strcmp(key, "Owner") == 0) {
        lua_newtable(L); // dummy
        return 1;
    } else if (strcmp(key, "AccessibilityLevel") == 0) {
        lua_getglobal(L, "Tracker");
        Tracker* tracker = Tracker::luaL_testthis(L, -1);
        if (!tracker) return 0;
        int res = (int)tracker->isReachable(*this);
        // NOTE: we don't support AccessibilityLevel::CLEARED for Locations yet
        lua_pushinteger(L, res);
        return 1;
    }
    return 0;
}

bool Location::Lua_NewIndex(lua_State *L, const char *key)
{
    return false;
}


std::list<Location> Location::FromJSON(
    json& j,
    const std::list<Location>& parentLookup,
    bool glitchedScoutableAsGlitched,
    const std::list< std::list<std::string> >& prevAccessRules,
    const std::list< std::list<std::string> >& prevVisibilityRules,
    const std::string& closedImgR,
    const std::string& openedImgR,
    const std::string& overlayBackgroundR,
    const std::string& parentName)
{
    // TODO: sine we store all intermediate locations now, we could pass a parent to FromJSON instead of all arguments
    std::list<Location> locs;
    
    if (j.type() == json::value_t::array) {
        for (auto& v : j) {
            for (auto& loc : FromJSON(
                v,
                parentLookup,
                glitchedScoutableAsGlitched,
                prevAccessRules,
                prevVisibilityRules,
                closedImgR,
                openedImgR,
                overlayBackgroundR,
                parentName
            )) {
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
    std::string parent = to_string(j["parent"], "");
    const Location* parentLocation = nullptr;
    if (!parent.empty()) {
        if (parent[0] == '@') parent = parent.substr(1);
        for (const auto& l : parentLookup) {
            // find exact match
            if (l.getID() == parent) {
                parentLocation = &l;
                break;
            }
        }
        if (!parentLocation) {
            for (const auto& l : parentLookup) {
                // fuzzy match
                const auto& id = l.getID();
                if (id.length() > parent.length() && id.compare(id.length() - parent.length(), parent.length(), parent)==0) {
                    parentLocation = &l;
                    break;
                }
            }
        }
        if (!parentLocation) {
            fprintf(stderr, "Location: did not find parent \"%s\" for \"%s\"\n",
                    sanitize_print(parent).c_str(), sanitize_print(name).c_str());
        }
    }

    const auto& parentAccessRules = parentLocation ? parentLocation->getAccessRules() : prevAccessRules;
    const auto& parentVisibilityRules = parentLocation ? parentLocation->getVisibilityRules() : prevVisibilityRules;

    std::list< std::list<std::string> > accessRules;
    if (j["access_rules"].is_array() && !j["access_rules"].empty()) {
        // TODO: merge code with Section's access rules
        for (const auto& v : j["access_rules"]) {
            std::list<std::string> newRule;
            if (!parseRule(v, newRule, "Location", "access", name))
                continue;
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
    } else if (j["access_rules"].is_string() && !j["access_rules"].empty()) {
        // single string access rule, same as [["code"]]
        const std::string& newTest = j["access_rules"];
        for (auto oldRule : parentAccessRules) {
            oldRule.push_back(newTest);
            accessRules.push_back(oldRule);
        }
        if (parentAccessRules.empty()) {
            accessRules.push_back({newTest});
        }
    } else {
        accessRules = parentAccessRules; // TODO: avoid copy
        if (!j["access_rules"].is_null() && !j["access_rules"].is_array()) {
            fprintf(stderr, "Location: invalid access rules in \"%s\"\n",
                    sanitize_print(name).c_str());
        }
    }
    std::list< std::list<std::string> > visibilityRules;
    if (j["visibility_rules"].is_array() && !j["visibility_rules"].empty()) {
        // TODO: merge code with Section's access rules
        for (const auto& v : j["visibility_rules"]) {
            std::list<std::string> newRule;
            if (!parseRule(v, newRule, "Location", "visibility", name))
                continue;
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
    } else if (j["visibility_rules"].is_string() && !j["visibility_rules"].empty()) {
        // single string visibility rule, same as [["code"]]
        const std::string& newTest = j["visibility_rules"];
        for (auto oldRule : parentVisibilityRules) {
            oldRule.push_back(newTest);
            visibilityRules.push_back(oldRule);
        }
        if (parentVisibilityRules.empty()) {
            visibilityRules.push_back({newTest});
        }
    } else {
        visibilityRules = parentVisibilityRules; // TODO: avoid copy
        if (!j["visibility_rules"].is_null() && !j["visibility_rules"].is_array()) {
            fprintf(stderr, "Location: invalid visibility rules in \"%s\"\n",
                    sanitize_print(name).c_str());
        }
    }
    
    std::string closedImg = to_string(j["chest_unopened_img"], closedImgR); // TODO: avoid copy
    std::string openedImg = to_string(j["chest_opened_img"], openedImgR);
    std::string overlayBackground = to_string(j["overlay_background"], overlayBackgroundR);
    glitchedScoutableAsGlitched = to_bool(j["inspectable_sequence_break"], glitchedScoutableAsGlitched);

    {
        Location loc;
        loc._name = name;
        loc._parentName = parentName;
        loc._id = loc._parentName.empty() ? loc._name : (loc._parentName + "/" + loc._name);
        loc._accessRules = accessRules;
        loc._visibilityRules = visibilityRules;
        loc._glitchedScoutableAsGlitched = glitchedScoutableAsGlitched;
        if (j["map_locations"].is_array()) {
            for (auto& v : j["map_locations"]) {
                if (v.type() != json::value_t::object) {
                    fprintf(stderr, "Location: bad map location\n");
                    continue;
                }
                loc._mapLocations.push_back(MapLocation::FromJSON(v));
            }
        } else if (!j["map_locations"].is_null()) {
            fprintf(stderr, "Location: invalid map locations\n");
        }
        if (j["sections"].is_array()) {
            for (auto& v: j["sections"]) {
                if (v.type() != json::value_t::object) {
                    fprintf(stderr, "Location: bad section\n");
                    continue;
                }
                loc._sections.push_back(LocationSection::FromJSON(
                    v,
                    loc._id,
                    glitchedScoutableAsGlitched,
                    accessRules,
                    visibilityRules,
                    closedImg,
                    openedImg,
                    overlayBackground));
            }
        } else if (!j["sections"].is_null()) {
            fprintf(stderr, "Location: invalid sections\n");
        }
        locs.push_back(loc);
    }

    if (j["children"].type() == json::value_t::array) {
        std::string fullname = parentName.empty() ? name : (parentName + "/" + name);
        for (auto& loc : FromJSON(
            j["children"],
            parentLookup,
            glitchedScoutableAsGlitched,
            accessRules,
            visibilityRules,
            closedImg,
            openedImg,
            overlayBackground,
            fullname
        )) {
            locs.push_back(std::move(loc));
        }
    } else if (j["children"].type() != json::value_t::null) {
        fprintf(stderr, "Location: bad children\n");
    }
    
    return locs;
}


void Location::merge(const Location& other)
{
    for (auto& maploc: other._mapLocations) {
        _mapLocations.push_back(maploc);
    }
    for (auto& sec: other._sections) {
        // TODO: detect duplicates and overwrite; NOTE: doing that would invalidate Section& whenever calling into Lua
        _sections.push_back(sec);
    }
}

Location::MapLocation Location::MapLocation::FromJSON(json& j)
{
    MapLocation maploc;
    maploc._mapName = to_string(j["map"],"");
    maploc._x = to_int(j["x"],0);
    maploc._y = to_int(j["y"],0);    
    maploc._size = to_int(j["size"],-1);
    maploc._borderThickness = to_int(j["border_thickness"],-1);
    maploc._shape = Location::MapLocation::ShapeFromString(to_string(j["shape"], ""));

    if (j["restrict_visibility_rules"].is_array()) {
        for (const auto& v : j["restrict_visibility_rules"]) {
            // outer array is logical Or, inner array (or string) is logical And
            std::list<std::string> newRule;
            if (!parseRule(v, newRule, "MapLocation", "restrict visibility", maploc._mapName))
                continue;
            maploc._visibilityRules.push_back(newRule);
        }
    } else if (j["restrict_visibility_rules"].is_string()) {
        const std::string& newTest = j["restrict_visibility_rules"];
        maploc._visibilityRules.push_back({newTest});
    } else if (!j["restrict_visibility_rules"].is_null()) {
        fprintf(stderr, "MapLocation: invalid restrict_visibility_rules for \"%s\"\n",
                sanitize_print(maploc._mapName).c_str());
    }

    if (j["force_invisibility_rules"].is_array()) {
        for (const auto& v : j["force_invisibility_rules"]) {
            // outer array is logical Or, inner array (or string) is logical And
            std::list<std::string> newRule;
            if (!parseRule(v, newRule, "MapLocation", "force invisibility", maploc._mapName))
                continue;
            maploc._invisibilityRules.push_back(newRule);
        }
    } else if (j["force_invisibility_rules"].is_string()) {
        const std::string& newTest = j["force_invisibility_rules"];
        maploc._invisibilityRules.push_back({newTest});
    } else if (!j["force_invisibility_rules"].is_null()) {
        fprintf(stderr, "MapLocation: invalid force_invisibility_rules for \"%s\"\n",
                sanitize_print(maploc._mapName).c_str());
    }

    return maploc;
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
