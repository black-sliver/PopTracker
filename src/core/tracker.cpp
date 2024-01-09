#include "tracker.h"
#include <luaglue/luamethod.h>
#include <cstring>
#include <nlohmann/json.hpp>
#include "jsonutil.h"
#include "util.h"
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

int Tracker::_execLimit = Tracker::DEFAULT_EXEC_LIMIT;

static LayoutNode blankLayoutNode = LayoutNode::FromJSON(json({}));
static JsonItem blankItem = JsonItem::FromJSON(json({}));
static Map blankMap = Map::FromJSON(json({}));
static Location blankLocation;// = Location::FromJSON(json({}));
static LocationSection blankLocationSection;// = LocationSection::FromJSON(json({}));

Tracker::Tracker(Pack* pack, lua_State *L)
    : _pack(pack), _L(L)
{
}

Tracker::~Tracker()
{
}

static const char* timeout_error_message = "Execution aborted. Limit reached.";

static int lua_error_handler(lua_State *L)
{
    // skip trace for certain errors unless running in debug mode (DEBUG == true)
    if (lua_isstring(L, -1) && strstr(lua_tostring(L, -1), timeout_error_message)) {
        lua_getglobal(L, "DEBUG");
        lua_pushboolean(L, true);
        if (!lua_compare(L, -1, -2, LUA_OPEQ)) {
            // return original error
            lua_pop(L, 2);
            return 1;
        }
    }
    // generate and print trace, then return original error
    luaL_traceback(L, L, NULL, 1);
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 1;
}

/// when used as count hook, aborts execution of user function after count instructions
static void lua_timeout_hook(lua_State *L, lua_Debug *)
{
    luaL_error(L, timeout_error_message);
}

// This functaion can be used internally when the return type is uncertain.
// Use carefully; the return value(s) and error handler will still be on the lua stack
static int RunLuaFunction_inner(lua_State *L, const std::string name, int execLimit)
{
    lua_pushcfunction(L, lua_error_handler);

    // Trim excess characters (such as $) and extract function name
    std::string workingString = name;
    if (workingString[0] == '$') {
        workingString = workingString.substr(1);
    }
    auto pos = workingString.find('|');
    std::string funcName = workingString.substr(0, pos);

    // Acquire the Lua function
    int t = lua_getglobal(L, funcName.c_str());
    if (t != LUA_TFUNCTION) {
        fprintf(stderr, "Missing Lua function for %s\n", name.c_str());
        lua_pop(L, 2); // non-function variable or nil, lua_error_handler
        return -1;
    }

    // Parse arguments, if any
    int argc = 0;
    if (pos < std::string::npos) {
        std::string::size_type next;
        while ((next = workingString.find('|', pos+1)) != std::string::npos) {
            lua_pushstring(L, workingString.substr(pos+1, next-pos-1).c_str());
            ++argc;
            pos = next;
        }
        // Get the last arg
        lua_pushstring(L, workingString.substr(pos+1).c_str());
        ++argc;
    }

    if (execLimit > 0)
        lua_sethook(L, lua_timeout_hook, LUA_MASKCOUNT, execLimit);
    auto res = lua_pcall(L, argc, 1, -argc-2);
    if (execLimit > 0)
        lua_sethook(L, nullptr, 0, 0);
    
    if (res != LUA_OK) {
        auto err = lua_tostring(L, -1);
        fprintf(stderr, "Error running %s:\n%s\n", name.c_str(), err ? err : "Unknown error");
        lua_pop(L, 2); // error object, lua_error_handler
    }

    return res;
}

/// Attempts to run a lua function by name and return an integer value
int Tracker::runLuaFunction(lua_State *L, const std::string name)
{
    int out = 0;
    auto callStatus = runLuaFunction(L, name, out);
    if (callStatus == LUA_OK)
        return out;
    else
        return 0;
}

int Tracker::runLuaFunction(lua_State* L, const std::string name, int &out)
{
    int callStatus = RunLuaFunction_inner(L, name, _execLimit);
    if (callStatus != LUA_OK) {
        // RunLuaFunction_inner handles popping the stack in case of errors
        return callStatus;
    }
    
    // This version of the function is set to accept a number if possible
    int isnum = 0;
    out = lua_tonumberx(L, -1, &isnum); // || (lua_isboolean(L, -1) && lua_toboolean(L, -1));
    if (!isnum && lua_isboolean(L, -1) && lua_toboolean(L, -1))
        out = 1;
    lua_pop(L, 2); // result, lua_error_handler
    
    return callStatus;
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
            if (!_bulkUpdate)
                _reachableCache.clear();
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
            if (_bulkUpdate)
                _bulkItemUpdates.push_back(i->getID());
            else
                onStateChanged.emit(this, i->getID());
        }};
        item.onDisplayChange += {this, [this](void* sender) {
            JsonItem* i = (JsonItem*)sender;
            onDisplayChanged.emit(this, i->getID());
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
        if (!item.getBaseItem().empty()) {
            // fire event for toggle_badged when base item changed
            auto id = item.getID();
            auto o = FindObjectForCode(item.getBaseItem().c_str());
            auto update = [this,id]() {
                for (auto& i : _jsonItems) {
                    if (i.getID() == id) {
                        i.onChange.emit(&i);
                        break;
                    }
                }
            };
            if (o.type == Object::RT::JsonItem) {
                o.jsonItem->onChange += {this, [update](void* sender) {
                    update();
                }};
            } else if (o.type == Object::RT::LuaItem) {
                o.luaItem->onChange += {this, [update](void* sender) {
                    update();
                }};
            }
        }
    }
    
    onLayoutChanged.emit(this, ""); // TODO: differentiate between structure and content
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
    _sectionRefs.clear();
    for (auto& loc : Location::FromJSON(j, _locations)) {
        // find duplicate, warn and merge
#ifdef MERGE_DUPLICATE_LOCATIONS // this should be default in the future
        bool merged = false;
        for (auto& other: _locations) {
            if (other.getID() == loc.getID()) {
                fprintf(stderr, "WARNING: merging duplicate location \"%s\"!\n", sanitize_print(loc.getID()).c_str());
                other.merge(loc);
                merged = true;
                for (auto& sec : other.getSections()) {
                    sec.onChange -= this;
                    sec.onChange += {this,[this,&sec](void*){ onLocationSectionChanged.emit(this, sec); }};
                }
                break;
            }
        }
        if (merged) continue;
#else
        bool duplicate = false;
        for (auto& other: _locations) {
            if (other.getID() == loc.getID()) {
                duplicate = true;
                break;
            }
        }
        if (duplicate) {
            std::string oldID = loc.getID();
            std::string newID;
            unsigned n = 1;
            while (newID.empty()) {
                newID = oldID + "[" + std::to_string(n) + "]";
                for (const auto& other: _locations) {
                    if (other.getID() == newID) {
                        newID.clear();
                        break;
                    }
                }
                n++;
            }
            loc.setID(newID);
            fprintf(stderr, "WARNING: renaming duplicate location \"%s\" to \"%s\"!\n"
                    "  This behavior will change in the future!\n",
                    sanitize_print(oldID).c_str(), sanitize_print(newID).c_str());
        }
#endif
        _locations.push_back(std::move(loc)); // TODO: move constructor
        for (auto& sec : _locations.back().getSections()) {
            if (!sec.getRef().empty())
                _sectionNameRefs[sec.getRef()].push_back(_locations.back().getID() + "/" + sec.getName());
            sec.onChange += {this,[this,&sec](void*){ onLocationSectionChanged.emit(this, sec); }};
        }
    }

    onLayoutChanged.emit(this, ""); // TODO: differentiate between structure and content
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
            fprintf(stderr, "Bad map\n");
            continue; // ignore
        }
        _maps[v["name"]] = Map::FromJSON(v);
    }
    
    onLayoutChanged.emit(this, ""); // TODO: differentiate between structure and content
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

    if (j.find("layouts") != j.end() && j["layouts"].is_object())
        j = j["layouts"]; // legacy layout file
    else if (j.find("type") != j.end() && j.find("content") != j.end()
            && j["type"].is_string() && (j["content"].is_array() || j["content"].is_object()))
        j = json{{"tracker_broadcast", j}}; // legacy broadcast_layout

    for (auto& [key,value] : j.items()) {
        if (value.type() != json::value_t::object) {
            fprintf(stderr, "Bad layout: %s (type %d)\n", key.c_str(), (int)value.type());
            continue; // ignore
        }
        if (_layouts.find(key) != _layouts.end())
            fprintf(stderr, "WARNING: replacing existing layout \"%s\"\n",
                    key.c_str());
        _layouts[key] = LayoutNode::FromJSON(value);
    }
    
    // TODO: fire for each named layout
    onLayoutChanged.emit(this, ""); // TODO: differentiate between structure and content
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
        int res = Tracker::runLuaFunction(_L, code);
        _providerCountCache[code] = res;
        return res;
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
    // TODO: locations (not just sections)?
    if (*code == '@') { // location section
        const char *start = code+1;
        const char *t = strrchr(start, '/');
        if (t) { // valid section identifier
            std::string locid = std::string(start, t-start);
            std::string secname = t+1;
            // match by ID (includes all parents)
            auto& loc = getLocation(locid, true);
            for (auto& sec: loc.getSections()) {
                if (sec.getName() != secname) continue;
                return &sec;
            }
            
        }
    }
    if (*code == '@') { // location (not section)
        const char *start = code+1;
        auto& loc = getLocation(start, true);
        if (!loc.getID().empty())
            return &loc;
    } else {
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
    }
    printf("Did not find object for code \"%s\".\n", sanitize_print(code).c_str());
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
    } else if (strcmp(key, "BulkUpdate") == 0) {
        lua_pushboolean(L, _bulkUpdate);
        return 1;
    } else {
        printf("Tracker::Lua_Index(\"%s\") unknown\n", key);
    }
    return 0;
}

bool Tracker::Lua_NewIndex(lua_State *L, const char* key) {
    if (strcmp(key, "ActiveVariantUID") == 0) {
        luaL_error(L, "Tried to write read-only property Tracker.%s", key);
    } else if (strcmp(key, "BulkUpdate") == 0) {
        bool val = lua_isnumber(L, -1) ? (lua_tonumber(L, -1) != 0) : lua_toboolean(L, -1);
        if (!val) {
            for (const auto& id: _bulkItemUpdates)
                onStateChanged.emit(this, id);
            _bulkItemUpdates.clear();
        }
        _bulkUpdate = val;
        return true;
    }
    return false;
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
BaseItem& Tracker::getItemById(const std::string& id)
{
    for (auto& item: _jsonItems) {
        if (item.getID() == id) return item;
    }
    for (auto& item: _luaItems) {
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
        if (id.find('/') == id.npos) {
            for (auto& loc : _locations) {
                if (loc.getName() == id)
                    return loc;
            }
        } else {
            std::string search = "/" + id;
            for (auto& loc : _locations) {
                const auto& s = loc.getID();
                if (s.size()>search.size() && s.compare(s.size()-search.size(), search.size(), search) == 0)
                    return loc;
            }
        }
    }
    return blankLocation;
}

std::pair<Location&, LocationSection&> Tracker::getLocationAndSection(const std::string& id)
{
    const char *start = id.c_str();
    const char *t = strrchr(start, '/');
    if (t) { // valid section identifier
        std::string locid = std::string(start, t-start);
        std::string secname = t+1;
        // match by ID (includes all parents)
        auto& loc = getLocation(locid, true);
        for (auto& sec: loc.getSections()) {
            if (sec.getName() != secname) continue;
            return {loc, sec};
        }
    }
    return {blankLocation, blankLocationSection};
}

LocationSection& Tracker::getLocationSection(const std::string& id)
{
    return getLocationAndSection(id).second;
}

const std::vector<std::pair<std::reference_wrapper<const Location>, std::reference_wrapper<const LocationSection>>>&
Tracker::getReferencingSections(const LocationSection& sec)
{
    static std::vector<std::pair<std::reference_wrapper<const Location>, std::reference_wrapper<const LocationSection>>>
    blank = {};

    if (_sectionRefs.empty() && !_sectionNameRefs.empty())
        rebuildSectionRefs();

    auto it = _sectionRefs.find(sec);
    if (it != _sectionRefs.end()) {
        return it->second;
    }
    return blank;
}

void Tracker::rebuildSectionRefs()
{
    _sectionRefs.clear();
    for (const auto& pair: _sectionNameRefs) {
        const auto& target = getLocationSection(pair.first);
        if (target.getName().empty())
            continue;
        for (const auto& sourceName: pair.second) {
            const auto& source = getLocationAndSection(sourceName);
            if (source.second.getRef().empty())
                continue;
            _sectionRefs[target].push_back(source);
        }
    }
}

const Pack* Tracker::getPack() const
{
    return _pack;
}

bool Tracker::changeItemState(const std::string& id, BaseItem::Action action)
{
    std::string baseCode; // for type: toggle_badged
    for (auto& item: _jsonItems) {
        if (item.getID() != id) continue;
        if (item.changeState(action)) {
            // NOTE: item fires onChanged
            return true;
        }
        baseCode = item.getBaseItem();
        break;
    }
    for (auto& item: _luaItems) {
        if (item.getID() != id) continue;
        if (item.changeState(action)) {
            // NOTE: item fires onChanged
            return true;
        }
        baseCode = item.getBaseItem();
        break;
    }
    if (!baseCode.empty()) {
        // for items that have a base item, propagate click
        // NOTE: the items have to subscribe to their base's onChanged
        auto baseObject = FindObjectForCode(baseCode.c_str());
        if (baseObject.type == Object::RT::JsonItem) {
            return baseObject.jsonItem->changeState(BaseItem::Action::Single);
        } else if (baseObject.type == Object::RT::LuaItem) {
            return baseObject.luaItem->changeState(BaseItem::Action::Single);
        }
    }
    return false; // nothing changed
}

AccessibilityLevel Tracker::isReachable(const Location& location, const LocationSection& section)
{
    if (_parents) {
        return isReachable(location, section, *_parents);
    } else {
        std::list<std::string> parents;
        return isReachable(location, section, parents);
    }
}

bool Tracker::isVisible(const Location& location, const LocationSection& section)
{
    std::list<std::string> parents;
    return isVisible(location, section, parents);
}

AccessibilityLevel Tracker::isReachable(const Location& location)
{
    if (_parents) {
        return isReachable(location, *_parents);
    } else {
        std::list<std::string> parents;
        return isReachable(location, parents);
    }
}

AccessibilityLevel Tracker::isReachable(const LocationSection& section)
{
    for (const auto& loc: _locations) {
        if (loc.getID() == section.getParentID()) {
            return isReachable(loc, section);
        }
    }
    return AccessibilityLevel::NONE;
}

bool Tracker::isVisible(const Location& location)
{
    std::list<std::string> parents;
    return isVisible(location, parents);
}

bool Tracker::isVisible(const Location::MapLocation& mapLoc)
{
    std::list<std::string> parents;
    if (!mapLoc.getInvisibilityRules().empty()
            && isReachable(mapLoc.getInvisibilityRules(), true, parents) != AccessibilityLevel::NONE)
        return false;
    auto res = isReachable(mapLoc.getVisibilityRules(), true, parents);
    return (res != AccessibilityLevel::NONE);
}

AccessibilityLevel Tracker::isReachable(const std::list< std::list<std::string> >& rules, bool visibilityRules, std::list<std::string>& parents)
{
    // TODO: return enum instead of int
    // returns 0 for unreachable, 1 for reachable, 2 for glitches required

    bool glitchedReachable = false;
    bool inspectOnlyReachable = false;
    if (rules.empty()) return AccessibilityLevel::NORMAL;
    for (const auto& ruleset : rules) { //<-- these are all to be ORed
        if (ruleset.empty()) return AccessibilityLevel::NORMAL; // any empty rule set means true
        AccessibilityLevel reachable = AccessibilityLevel::NORMAL;
        bool inspectOnly = false;
        for (const auto& rule: ruleset) { //<-- these are all to be ANDed
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
                inspectOnly = true;
                s = s.substr(1,s.length()-1);
            }
            if (inspectOnly && s.length() > 0 && s[s.length()-1] == '}') {
                s = s.substr(0, s.length()-1);
            }
            if (inspectOnly && s.empty()) {
                inspectOnlyReachable = true;
                continue;
            }
            // '<rule>:<count>' checks count (e.g. consumables) instead of bool
            int count = 1;
            auto p = s.find(':');
            if (p != s.npos) {
                count = atoi(s.c_str()+p+1);
                s = s.substr(0,p);
            }
            // '^$func' gives direct accessibility level rather than an integer code count
            bool isAccessibilitLevel = s[0] == '^'; // use value as level instead of count
            if (isAccessibilitLevel) {
                if (s.length() < 3 || s[1] != '$') { // only ^$ supported
                    fprintf(stderr, "Warning: invalid rule \"%s\"",
                            sanitize_print(s).c_str());
                    reachable = AccessibilityLevel::NONE;
                    break;
                }
                s = s.substr(1);
            }
            // check cache for '@' rules
            auto it = _reachableCache.find(s);
            if (it != _reachableCache.end()) {
                { // value is glitched/not glitched
                    auto sub = it->second;
                    if (!inspectOnly && sub == AccessibilityLevel::INSPECT)
                        sub = AccessibilityLevel::NONE; // or set inspectOnly = true?
                    else if (optional && sub == AccessibilityLevel::NONE)
                        sub = AccessibilityLevel::SEQUENCE_BREAK;
                    else if (sub == AccessibilityLevel::NONE)
                        reachable = AccessibilityLevel::NONE;
                    if (sub == AccessibilityLevel::SEQUENCE_BREAK && reachable != AccessibilityLevel::NONE)
                        reachable = AccessibilityLevel::SEQUENCE_BREAK;
                    if (reachable == AccessibilityLevel::NONE)
                        break;
                }
            }
            // '@' references other locations
            else if (s[0] == '@') {
                const char* start = s.c_str()+1;
                const char* t = strrchr(s.c_str()+1, '/');
                std::string locid = s.substr(1);
                auto& loc = getLocation(locid, true);
                bool match = false;
                AccessibilityLevel sub = AccessibilityLevel::NONE;
                if (!t && loc.getID().empty()) {
                    printf("Invalid location %s for access rule!\n",
                            sanitize_print(s).c_str());
                    continue; // invalid location
                } else if (!loc.getID().empty()) {
                    // @-Rule for location, not a section
                    if (visibilityRules)
                        sub = isVisible(loc, parents) ? AccessibilityLevel::NORMAL : AccessibilityLevel::NONE;
                    else
                        sub = isReachable(loc, parents);
                    match = true;
                } else {
                    // @-Rule for a section (also run for missing location)
                    std::string sublocid = locid.substr(0, t-start);
                    std::string subsecname = t+1;
                    auto& subloc = getLocation(sublocid, true);
                    for (auto& subsec: subloc.getSections()) {
                        if (subsec.getName() != subsecname)
                            continue;
                        if (visibilityRules)
                            sub = isVisible(subloc, subsec, parents) ? AccessibilityLevel::NORMAL : AccessibilityLevel::NONE;
                        else
                            sub = isReachable(subloc, subsec, parents);
                        match = true;
                        break;
                    }
                }
                if (match) {
                    if (!visibilityRules)
                        _reachableCache[s] = sub; // only cache isReachable (not isVisible) for @
                    // combine current state with sub-result
                    if (!inspectOnly && sub == AccessibilityLevel::INSPECT)
                        sub = AccessibilityLevel::NONE; // or set checkable = true?
                    else if (optional && sub == AccessibilityLevel::NONE)
                        sub = AccessibilityLevel::SEQUENCE_BREAK;
                    else if (sub == AccessibilityLevel::NONE)
                        reachable = AccessibilityLevel::NONE;
                    if (sub == AccessibilityLevel::SEQUENCE_BREAK && reachable != AccessibilityLevel::NONE)
                        reachable = AccessibilityLevel::SEQUENCE_BREAK;
                } else {
                    printf("Could not find location %s for access rule!\n",
                            sanitize_print(s).c_str());
                }
                if (reachable == AccessibilityLevel::NONE) break;
            }
            // '$' calls into Lua, now also supported by ProviderCountForCode
            // other: references codes (with or without count)
            else {
                // NOTE: we don't use _reachableCache here, instead ProvideCountForCode has a cache
                _parents = &parents;
                int n = ProviderCountForCode(s);
                _parents = nullptr;
                if (isAccessibilitLevel) { // TODO: merge this with '@' code path
                    AccessibilityLevel sub = (AccessibilityLevel)n;
                    if (!inspectOnly && sub == AccessibilityLevel::INSPECT)
                        inspectOnly = true;
                    else if (optional && sub == AccessibilityLevel::NONE)
                        sub = AccessibilityLevel::SEQUENCE_BREAK;
                    else if (sub == AccessibilityLevel::NONE)
                        reachable = AccessibilityLevel::NONE;
                    if (sub == AccessibilityLevel::SEQUENCE_BREAK && reachable != AccessibilityLevel::NONE)
                        reachable = AccessibilityLevel::SEQUENCE_BREAK;
                    if (reachable == AccessibilityLevel::NONE)
                        break;
                    continue;
                }
                if (n >= count)
                    continue;
                if (optional) {
                    reachable = AccessibilityLevel::SEQUENCE_BREAK;
                } else {
                    reachable = AccessibilityLevel::NONE;
                    break;
                }
            }
        }
        if (reachable == AccessibilityLevel::NORMAL && !inspectOnly)
            return AccessibilityLevel::NORMAL;
        else if (reachable != AccessibilityLevel::NONE /*== 1*/ && inspectOnly)
            inspectOnlyReachable = true;
        if (reachable == AccessibilityLevel::SEQUENCE_BREAK)
            glitchedReachable = true;
    }
    return (glitchedReachable && !inspectOnlyReachable) ? AccessibilityLevel::SEQUENCE_BREAK :
           (inspectOnlyReachable && !glitchedReachable) ? AccessibilityLevel::INSPECT :
               AccessibilityLevel::NONE;
}

AccessibilityLevel Tracker::isReachable(const Location& location, const LocationSection& section, std::list<std::string>& parents)
{
    const LocationSection& realSection = section.getRef().empty() ? section : getLocationSection(section.getRef());
    std::string id = realSection.getParentID() + "/" + realSection.getName();
    if (std::count(parents.begin(), parents.end(), id) > 1) {
        printf("access_rule recursion detected: %s!\n", id.c_str());
        // returning 0 here should mean this path is unreachable, other paths that are logical "or" should be resolved
        return AccessibilityLevel::NONE;
    }
    parents.push_back(id);
    auto res = isReachable(realSection.getAccessRules(), false, parents);
    parents.pop_back();
    return res;
}

bool Tracker::isVisible(const Location& location, const LocationSection& section, std::list<std::string>& parents)
{
    const LocationSection& realSection = section.getRef().empty() ? section : getLocationSection(section.getRef());
    std::string id = realSection.getParentID() + "/" + realSection.getName();
    if (std::count(parents.begin(), parents.end(), id) > 1) {
        printf("visibility_rule recursion detected: %s!\n", id.c_str());
        return 0;
    }
    parents.push_back(id);
    auto res = isReachable(realSection.getVisibilityRules(), true, parents);
    parents.pop_back();
    return (res != AccessibilityLevel::NONE);
}

AccessibilityLevel Tracker::isReachable(const Location& location, std::list<std::string>& parents)
{
    if (std::count(parents.begin(), parents.end(), location.getID()) > 1) {
        printf("access_rule recursion detected: %s!\n", location.getID().c_str());
        return AccessibilityLevel::NONE;
    }
    parents.push_back(location.getID());
    auto res = isReachable(location.getAccessRules(), false, parents);
    parents.pop_back();
    return res;
}

bool Tracker::isVisible(const Location& location, std::list<std::string>& parents)
{
    if (std::count(parents.begin(), parents.end(), location.getID()) > 1) {
        printf("visibility_rule recursion detected: %s!\n", location.getID().c_str());
        return 0;
    }
    parents.push_back(location.getID());
    auto res = isReachable(location.getVisibilityRules(), true, parents);
    parents.pop_back();
    return (res != AccessibilityLevel::NONE);
}

LuaItem * Tracker::CreateLuaItem()
{
    _luaItems.push_back({});
    LuaItem& i = _luaItems.back();
    i.setID(++_lastItemID);
    i.onChange += {this, [this](void* sender) {
        if (!_bulkUpdate)
            _reachableCache.clear();
        _providerCountCache.clear();
        LuaItem* i = (LuaItem*)sender;
        if (_bulkUpdate)
            _bulkItemUpdates.push_back(i->getID());
        else
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
                fprintf(stderr, "WARNING: duplicate location section: \"%s\"!\n",
                        sanitize_print(id).c_str());
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
    _bulkItemUpdates.clear();
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
    for (const auto& id: _bulkItemUpdates)
        onStateChanged.emit(this, id);
    _bulkItemUpdates.clear();
    _bulkUpdate = false;

    return true;
}

void Tracker::setExecLimit(int execLimit)
{
    _execLimit = execLimit;
}
