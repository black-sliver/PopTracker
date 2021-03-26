#include "tracker.h"
#include "../luaglue/luamethod.h"
#include <cstring>
#include <json/json.hpp>
#include "jsonutil.h"
using nlohmann::json;

const LuaInterface<Tracker>::MethodMap Tracker::Lua_Methods = {
    LUA_METHOD(Tracker, AddItems, const char*),
    LUA_METHOD(Tracker, AddLocations, const char*),
    LUA_METHOD(Tracker, AddMaps, const char*),
    LUA_METHOD(Tracker, AddLayouts, const char*),
    LUA_METHOD(Tracker, ProviderCountForCode, const char*),
    LUA_METHOD(Tracker, FindObjectForCode, const char*),
};

static LayoutNode blankLayoutNode = LayoutNode::FromJSON(json({}));
static BaseItem blankItem = JsonItem::FromJSON(json({}));
static Map blankMap = Map::FromJSON(json({}));
static Location blankLocation;// = Location::FromJSON(json({}));

Tracker::Tracker(Pack* pack, lua_State *L)
    : _pack(pack), _L(L)
{
}

Tracker::~Tracker()
{
}

bool Tracker::AddItems(const std::string& file) {
    printf("Loading items from \"%s\"...\n", file.c_str());
    std::string s;
    if (!_pack->ReadFile(file, s)) return false; // TODO: throw lua error?
    json j = parse_jsonc(s);
    
    if (j.type() != json::value_t::array) {
        fprintf(stderr, "Bad json\n"); // TODO: throw lua error?
        return false;
    }
    
    for (auto& v : j) {
        if (v.type() != json::value_t::object) {
            fprintf(stderr, "Bad item\n");
            continue; // ignore
        }
        _jsonItems.push_back(JsonItem::FromJSON(v));
        _jsonItems.back().setID(++_lastItemID);
        _jsonItems.back().onChange += {this, [this](void* sender) {
            JsonItem* i = (JsonItem*)sender;
            onStateChanged.emit(this, i->getID());
        }};
    }
    
    onLayoutChanged.emit(this,""); // TODO: differentiate between structure and content
    return true;
}
bool Tracker::AddLocations(const std::string& file) {
    printf("Loading locations from \"%s\"...\n", file.c_str());
    std::string s;
    if (!_pack->ReadFile(file, s)) return false; // TODO: throw lua error?
    json j = parse_jsonc(s);
    
    if (j.type() != json::value_t::array) {
        fprintf(stderr, "Bad json\n"); // TODO: throw lua error?
        return false;
    }
    
    for (auto& loc : Location::FromJSON(j)) {
        //loc.dump(true);
        _locations.push_back(std::move(loc)); // TODO: move constructor
        for (auto& sec : _locations.back().getSections()) {
            sec.onChange += {this,[this,&sec](void*){ onLocationSectionChanged.emit(this, sec); }};
        }
    }
    
    onLayoutChanged.emit(this,""); // TODO: differentiate between structure and content
    return false;
}
bool Tracker::AddMaps(const std::string& file) {
    printf("Loading maps from \"%s\"...\n", file.c_str());
    std::string s;
    if (!_pack->ReadFile(file, s)) return false; // TODO: throw lua error?
    json j = parse_jsonc(s);
    
    if (j.type() != json::value_t::array) {
        fprintf(stderr, "Bad json\n"); // TODO: throw lua error?
        return false;
    }
    
    for (auto& v : j) {
        if (v.type() != json::value_t::object || v["name"].type() != json::value_t::string) {
            fprintf(stderr, "Bad item\n");
            continue; // ignore
        }
        _maps[v["name"]] = Map::FromJSON(v);
    }
    
    onLayoutChanged.emit(this,""); // TODO: differentiate between structure and content
    return false;
}
bool Tracker::AddLayouts(const std::string& file) {
    printf("Loading layouts from \"%s\"...\n", file.c_str());
    std::string s;
    if (!_pack->ReadFile(file, s)) return false; // TODO: throw lua error?
    json j = parse_jsonc(s);
    
    if (j.type() != json::value_t::object) {
        fprintf(stderr, "Bad json\n"); // TODO: throw lua error?
        return false;
    }
    
    for (auto& [key,value] : j.items()) {
        if (value.type() != json::value_t::object) {
            fprintf(stderr, "Bad item\n");
            continue; // ignore
        }
        _layouts[key] = LayoutNode::FromJSON(value);
    }
    
    // TODO: fire for each named layout
    onLayoutChanged.emit(this,""); // TODO: differentiate between structure and content
    return false;
}
int Tracker::ProviderCountForCode(const std::string& code)
{
    int res=0;
    for (const auto& item : _jsonItems)
    {
        res += item.providesCode(code);
    }
    for (const auto& item : _luaItems)
    {
        res += item.providesCode(code);
    }
    return res;
}
Tracker::Object Tracker::FindObjectForCode(const char* code)
{
    // TODO: non-lua item, locations
    if (*code == '@') { // location section
        const char *start = code+1;
        const char *t = strrchr(start, '/');
        if (t) { // valid section identifier
            std::string locid = std::string(start, t-start);
            std::string secname = t+1;
            // match by ID (includes all parents)
            auto& loc = getLocation(locid, true);
            printf("%s => %s => %s [%d]\n", code, locid.c_str(), loc.getName().c_str(), (int)loc.getSections().size());
            for (auto& sec: loc.getSections()) {
                if (sec.getName() != secname) continue;
                return &sec;
            }
            
        }
    }
    for (auto& item : _jsonItems) {
        //if (item.providesCode(code)) {
        if (item.canProvideCode(code)) {
            return &item;
        }
    }
    for (auto& item : _luaItems) {
        //if (item.providesCode(code)) {
        if (item.canProvideCode(code)) {
            return &item;
        }
    }
    printf("Did not find object for code \"%s\".\n", code);
    return nullptr;
}

int Tracker::Lua_Index(lua_State *L, const char* key) {
    if (strcmp(key, "ActiveVariantUID") == 0) {
        lua_pushstring(L, _pack->getVariant().c_str());
        return 1;
    } else {
        printf("Tracker::Lua_Index(\"%s\") unknown\n", key);
    }
    return 0;
}

const Map& Tracker::getMap(const std::string& name) const
{
    const auto it = _maps.find(name);
    return it==_maps.end() ? blankMap : it->second;
}

std::list<std::string> Tracker::getMapNames() const
{
    std::list<std::string> res;
    for (const auto& pair : _maps) res.push_back(pair.first);
    return res;
}

const LayoutNode& Tracker::getLayout(const std::string& name) const
{
    const auto it = _layouts.find(name);
    return it==_layouts.end() ? blankLayoutNode : it->second;
}
bool Tracker::hasLayout(const std::string& name) const
{
    return _layouts.find(name) != _layouts.end();
}
const BaseItem& Tracker::getItemByCode(const std::string& code) const
{
    for (const auto& item: _jsonItems) {
        if (item.canProvideCode(code)) return item;
    }
    
    for (const auto& item: _luaItems) {
        if (item.canProvideCode(code)) return item;
    }
    
    return blankItem;
}
const BaseItem& Tracker::getItemById(const std::string& id) const
{
    for (const auto& item: _jsonItems) {
        if (item.getID() == id) return item;
    }
    for (const auto& item: _luaItems) {
        if (item.getID() == id) return item;
    }
    return blankItem;
}
std::list< std::pair<std::string, Location::MapLocation> > Tracker::getMapLocations(const std::string& mapname) const
{
    std::list< std::pair<std::string, Location::MapLocation> > res;
    for (const auto& loc : _locations) {
        for (const auto& maploc : loc.getMapLocations()) {
            if (maploc.getMap() == mapname) {
                res.push_back( std::make_pair(loc.getID(), maploc) );
            }
        }
    }
    return res;
}
Location& Tracker::getLocation(const std::string& id, bool partialMatch)
{
    for (auto& loc : _locations) {
        if (loc.getID() == id)
            return loc;
    }
    if (partialMatch) {
        for (auto& loc : _locations) {
            if (loc.getName() == id)
                return loc;
        }
    }
    return blankLocation;
}

const Pack* Tracker::getPack() const
{
    return _pack;
}

bool Tracker::changeItemState(const std::string& id, BaseItem::Action action)
{
    for (auto& item: _jsonItems) {
        if (item.getID() != id) continue;
        if (item.changeState(action)) {
            // NOTE: item fires onChanged
            return true;
        }
        break;
    }
    for (auto& item: _luaItems) {
        if (item.getID() != id) continue;
        if (item.changeState(action)) {
            // NOTE: item fires onChanged
            return true;
        }
        break;
    }
    return false; // nothing changed
}

int Tracker::isReachable(const LocationSection& section)
{
    // TODO: build a cache for this (flush cache after update of map(s))
    // TODO: return enum instead of int
    // returns 0 for unreachable, 1 for reachable, 2 for glitches required
    
    bool glitchedReachable = false;
    bool checkOnlyReachable = false;
    const auto& rules = section.getRules();
    if (rules.empty()) return 1;
    for (const auto& ruleset : rules) { //<-- these are all to be ORed
        if (ruleset.empty()) return 1; // any empty rule set means true
        int reachable = 1;
        bool checkOnly = false;
        for (const auto& rule: ruleset) { // <-- these are all to be ANDed
            if (rule.empty()) continue; // empty/missing code is true
            std::string s = rule;
            // '[' ... ']' means optional/glitches required (different color)
            bool optional = false;
            if (s.length() > 1 && s[0] == '[' && s[s.length()-1]==']') {
                optional = true;
                s = s.substr(1,s.length()-2);
            }
            // '{' ... '}' means required to check (i.e. the rule never returns "reachable", but "checkable" instead)
            if (s.length() > 1 && s[0] == '{') {
                checkOnly = true;
                s = s.substr(1,s.length()-1);
            }
            if (checkOnly && s.length() > 0 && s[s.length()-1] == '}') {
                s = s.substr(0, s.length()-1);
            }
            if (checkOnly && s.empty()) {
                checkOnlyReachable = true;
                continue;
            }
            // '<rule>:<count>' checks count (e.g. consumables) instead of bool
            int count = 1;
            auto p = s.find(':');
            if (p != s.npos) {
                count = atoi(s.c_str()+p+1);
                s = s.substr(0,p);
            }
            // '$' calls into lua
            if (s[0] == '$') {
                // TODO: use a helper to access lua instead of having lua state here
                lua_getglobal(_L, s.c_str()+1);
                if (lua_pcall(_L, 0, 1, 0) != LUA_OK) {
                    fprintf(stderr, "Error running $%s:\n%s\n",
                        s.c_str()+1, lua_tostring(_L,-1));
                    // TODO: clean up lua stack?
                } else if (lua_tonumber(_L, -1)>=count) {
                    lua_pop(_L,1);
                    continue;
                } else {
                    lua_pop(_L,1);
                }
                if (optional) {
                    reachable = 2;
                } else {
                    reachable = 0;
                    break;
                }
            }
            // '@' references other locations
            else if (s[0] == '@') {
                const char* start = s.c_str()+1;
                const char* t = strrchr(s.c_str()+1, '/');
                if (!t) continue; // invalid location
                std::string sublocid = s.substr(1, t-start);
                std::string subsecname = t+1;
                auto& subloc = getLocation(sublocid, true);
                for (auto& subsec: subloc.getSections()) {
                    if (subsec.getName() != subsecname) continue;
                    int sub = isReachable(subsec);
                    if (!checkOnly && sub==3) sub=0; // or set checkable = true?
                    else if (optional && sub) sub=2;
                    if (sub==2) reachable = 2;
                    if (sub==0) reachable = 0;
                    break;
                }
                if (!reachable) break;
            }
            // other: references codes (with count)
            else if (count>1) {
                if (ProviderCountForCode(s) >= count) continue;
                if (optional) {
                    reachable = 2;
                } else {
                    reachable = 0;
                    break;
                }
            }
            // (without count, optimized)
            else {
                bool match = false;
                for (const auto& item: _jsonItems) {
                    if (item.providesCode(s)) {
                        match = true;
                        break;
                    }
                }
                if (match) continue;
                if (optional) {
                    reachable = 2;
                } else {
                    reachable = 0;
                    break;
                }
            }
        }
        if (reachable==1 && !checkOnly) return 1;
        else if (reachable /*== 1*/ && checkOnly) checkOnlyReachable = true;
        if (reachable==2) glitchedReachable = true; 
    }
    return glitchedReachable ? 2 : checkOnlyReachable ? 3 : 0;
}

LuaItem * Tracker::CreateLuaItem()
{
    _luaItems.push_back({});
    LuaItem& i = _luaItems.back();
    i.setID(++_lastItemID);
    i.onChange += {this, [this](void* sender) {
        LuaItem* i = (LuaItem*)sender;
        onStateChanged.emit(this, i->getID());
    }};
    return &i;
}


json Tracker::saveState() const
{
    /* structure:
     { tracker: {
        format_version: 1,
        json_items: { "some_id": <JsonItem::saveState>, ... },
        lua_items: { "some_id": <LuaItem::saveState>, ... },
        sections: { "full_location_path": <LocationSection::saveState>, ... }
     } }
    */
    
    json jJsonItems = {};
    for (auto& item: _jsonItems) {
        jJsonItems[item.getID()] = item.save();
    }
    json jLuaItems = {};
    for (auto& item: _luaItems) {
        jLuaItems[item.getID()] = item.save();
    }
    json jSections = {};
    for (auto& loc: _locations) {
        for (auto& sec: loc.getSections()) {
            std::string id = loc.getID() + "/" + sec.getName();
            if (jSections.find(id) != jSections.end()) {
                fprintf(stderr, "WARN: duplicate location: \"%s\"!\n",
                        id.c_str());
            }
            jSections[id] = sec.save();
        }
    }
    
    json state = { {
        "tracker", {
            {"format_version", 1},
            {"json_items", jJsonItems},
            {"lua_items", jLuaItems},
            {"sections", jSections}
        }
    } };
    
    return state;
}

bool Tracker::loadState(nlohmann::json& state)
{
    if (state.type() != json::value_t::object) return false;
    auto& j = state["tracker"]; // state's tracker data
    if (j["format_version"] != 1) return false; // incompatible state format
    
    auto& jJsonItems = j["json_items"];
    if (jJsonItems.type() == json::value_t::object) {
        for (auto it=jJsonItems.begin(); it!=jJsonItems.end(); it++) {
            for (auto& item: _jsonItems) {
                if (item.getID() != it.key()) continue;
                item.load(it.value());
            }
        }
    }
    auto& jLuaItems = j["lua_items"];
    if (jLuaItems.type() == json::value_t::object) {
        for (auto it=jLuaItems.begin(); it!=jLuaItems.end(); ++it) {
            for (auto& item: _luaItems) {
                if (item.getID() != it.key()) continue;
                item.load(it.value());
            }
        }
    }
    auto& jSections = j["sections"];
    if (jSections.type() == json::value_t::object) {
        for (auto it=jSections.begin(); it!=jSections.end(); ++it) {
            std::string key = it.key();
            for (auto& loc: _locations) {
                if (key.rfind(loc.getID(), 0) != 0) continue;
                for (auto& sec: loc.getSections()) {
                    if (key != loc.getID() + "/" + sec.getName()) continue;
                    sec.load(it.value());
                }
            }
        }
    }
    
    return true;
}