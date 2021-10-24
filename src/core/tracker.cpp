#include "tracker.h"
#include "../luaglue/luamethod.h"
#include <cstring>
#include <nlohmann/json.hpp>
#include "jsonutil.h"
using nlohmann::json;

const LuaInterface<Tracker>::MethodMap Tracker::Lua_Methods = {
    LUA_METHOD(Tracker, AddItems, const char*),
    LUA_METHOD(Tracker, AddLocations, const char*),
    LUA_METHOD(Tracker, AddMaps, const char*),
    LUA_METHOD(Tracker, AddLayouts, const char*),
    LUA_METHOD(Tracker, ProviderCountForCode, const char*),
    LUA_METHOD(Tracker, FindObjectForCode, const char*),
    LUA_METHOD(Tracker, UiHint, const char*, const char*),
};

static LayoutNode blankLayoutNode = LayoutNode::FromJSON(json({}));
static JsonItem blankItem = JsonItem::FromJSON(json({}));
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
    
    _reachableCache.clear();
    _providerCountCache.clear();
    for (auto& v : j) {
        if (v.type() != json::value_t::object) {
            fprintf(stderr, "Bad item\n");
            continue; // ignore
        }
        _jsonItems.push_back(JsonItem::FromJSON(v));
        auto& item = _jsonItems.back();
        item.setID(++_lastItemID);
        item.onChange += {this, [this](void* sender) {
            if (!_bulkUpdate) _reachableCache.clear();
            _providerCountCache.clear();
            JsonItem* i = (JsonItem*)sender;
            if (i->getType() == BaseItem::Type::COMPOSITE_TOGGLE) {
                // update part items when changing composite
                unsigned n = (unsigned)i->getActiveStage();
                auto leftCodes = i->getCodes(1);
                auto rightCodes = i->getCodes(2);
                if (!leftCodes.empty()) {
                    auto o = FindObjectForCode(leftCodes.front().c_str());
                    if (o.type == Object::RT::JsonItem)
                        o.jsonItem->setState((n&1)?1:0); // TODO: advanceToCode() instead?
                    else if (o.type == Object::RT::LuaItem)
                        o.luaItem->setState((n&1)?1:0);
                }
                if (!rightCodes.empty()) {
                    auto o = FindObjectForCode(rightCodes.front().c_str());
                    if (o.type == Object::RT::JsonItem)
                        o.jsonItem->setState((n&2)?1:0);
                    else if (o.type == Object::RT::LuaItem)
                        o.luaItem->setState((n&2)?1:0);
                }
            }
            onStateChanged.emit(this, i->getID());
        }};
        if (item.getType() == BaseItem::Type::COMPOSITE_TOGGLE) {
            // update composite when changing part items (and get initial state)
            int n = 0;
            auto id = item.getID();
            auto leftCodes = item.getCodes(1);
            auto rightCodes = item.getCodes(2);
            auto update = [this,id](unsigned bit, bool value) {
                for (auto& i : _jsonItems) {
                    if (i.getID() == id) {
                        unsigned stage = (unsigned)i.getActiveStage();
                        i.setState(1, value ? (stage|bit) : (stage&~bit));
                        break;
                    }
                }
            };
            if (!leftCodes.empty()) {
                auto o = FindObjectForCode(leftCodes.front().c_str());
                if (o.type == Object::RT::JsonItem) {
                    if (o.jsonItem->getState()) n += 1;
                    o.jsonItem->onChange += { this, [update](void* sender) {
                        update(1, ((JsonItem*)sender)->getState());
                    }};
                }
                else if (o.type == Object::RT::LuaItem) {
                    if (o.luaItem->getState()) n += 1;
                    o.luaItem->onChange += { this, [update](void* sender) {
                        update(1, ((LuaItem*)sender)->getState());
                    }};
                }
            }
            if (!rightCodes.empty()) {
                auto o = FindObjectForCode(rightCodes.front().c_str());
                if (o.type == Object::RT::JsonItem) {
                    if (o.jsonItem->getState()) n += 2;
                    o.jsonItem->onChange += { this, [update](void* sender) {
                        update(2, ((JsonItem*)sender)->getState());
                    }};
                }
                else if (o.type == Object::RT::LuaItem) {
                    if (o.luaItem->getState()) n += 2;
                    o.luaItem->onChange += { this, [update](void* sender) {
                        update(2, ((LuaItem*)sender)->getState());
                    }};
                }
            }
            item.setState(1, n);
        }
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
    
    _reachableCache.clear();
    _providerCountCache.clear();
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
    // cache this, because inefficient use can make the Lua script hang
    auto it = _providerCountCache.find(code);
    if (it != _providerCountCache.end())
        return it->second;
    // "codes" starting with $ run Lua functions
    if (!code.empty() && code[0] == '$') {
        // TODO: use a helper to access lua instead of having lua state here
        int args = 0;
        auto pos = code.find('|');
        if (pos == code.npos) {
            lua_getglobal(_L, code.c_str()+1);
        } else {
            lua_getglobal(_L, code.substr(1, pos-1).c_str());
            std::string::size_type next;
            while ((next = code.find('|', pos+1)) != code.npos) {
                lua_pushstring(_L, code.substr(pos+1, next-pos-1).c_str());
                args++;
                pos = next;
            }
            lua_pushstring(_L, code.substr(pos+1).c_str());
            args++;
        }
        if (lua_pcall(_L, args, 1, 0) != LUA_OK) {
            fprintf(stderr, "Error running $%s:\n%s\n",
                code.c_str()+1, lua_tostring(_L,-1));
            // TODO: clean up lua stack?
            _providerCountCache[code] = 0;
            return 0;
        } else {
            int n = lua_tonumber(_L, -1);
            lua_pop(_L,1);
            _providerCountCache[code] = n;
            return n;
        }
    }
    // other codes count items
    int res=0;
    for (const auto& item : _jsonItems)
    {
        res += item.providesCode(code);
    }
    for (const auto& item : _luaItems)
    {
        res += item.providesCode(code);
    }
    _providerCountCache[code] = res;
    return res;
}
Tracker::Object Tracker::FindObjectForCode(const char* code)
{
    // TODO: locations
    if (*code == '@') { // location section
        const char *start = code+1;
        const char *t = strrchr(start, '/');
        if (t) { // valid section identifier
            std::string locid = std::string(start, t-start);
            std::string secname = t+1;
            // match by ID (includes all parents)
            auto& loc = getLocation(locid, true);
#if 0
            printf("%s => %s => %s [%d]\n", code, locid.c_str(), loc.getName().c_str(), (int)loc.getSections().size());
#endif
            for (auto& sec: loc.getSections()) {
                if (sec.getName() != secname) continue;
                return &sec;
            }
            
        }
    }
    for (auto& item : _jsonItems) {
        if (item.canProvideCode(code)) {
            return &item;
        }
    }
    for (auto& item : _luaItems) {
        if (item.canProvideCode(code)) {
            return &item;
        }
    }
    printf("Did not find object for code \"%s\".\n", code);
    return nullptr;
}

void Tracker::UiHint(const std::string& name, const std::string& value)
{
    onUiHint.emit(this, name, value);
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
            auto it = _reachableCache.find(s);
            if (it != _reachableCache.end()) {
                if (s[0] != 0) { // value is count
                    if (it->second >= count) continue;
                    if (optional) {
                        reachable = 2;
                    } else {
                        reachable = 0;
                        break;
                    }
                } else { // value is glitched/not glitched
                    int sub = it->second;
                    if (!checkOnly && sub==3) sub=0; // or set checkable = true?
                    else if (optional && sub) sub=2;
                    if (sub==2) reachable = 2;
                    if (sub==0) reachable = 0;
                    if (!reachable) break;
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
                    _reachableCache[s] = sub;
                    if (!checkOnly && sub==3) sub=0; // or set checkable = true?
                    else if (optional && sub) sub=2;
                    if (sub==2) reachable = 2;
                    if (sub==0) reachable = 0;
                    break;
                }
                if (!reachable) break;
            }
            // '$' calls into Lua, now also supported by ProviderCountForCode
            // other: references codes (with or without count)
            else {
                int n = ProviderCountForCode(s);
                _reachableCache[s] = n;
                if (n >= count) continue;
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
        if (!_bulkUpdate) _reachableCache.clear();
        _providerCountCache.clear();
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
    _reachableCache.clear();
    _providerCountCache.clear();
    if (state.type() != json::value_t::object) return false;
    auto& j = state["tracker"]; // state's tracker data
    if (j["format_version"] != 1) return false; // incompatible state format
    
    _bulkUpdate = true;
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
    _bulkUpdate = false;
    
    return true;
}
