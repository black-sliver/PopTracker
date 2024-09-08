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

int Tracker::luaErrorHandler(lua_State *L)
{
    // skip trace for certain errors unless running in debug mode (DEBUG == true or "errors" in DEBUG)
    if (lua_isstring(L, -1) && strstr(lua_tostring(L, -1), timeout_error_message)) {
        bool skip_trace;
        int t = lua_getglobal(L, "DEBUG");
        if (t == LUA_TNIL) {
            skip_trace = true;
        } else if (t == LUA_TTABLE) {
            lua_pushnil(L);
            skip_trace = true;
            while (lua_next(L, -2) != 0) {
                if (strcmp(lua_tostring(L, -1), "errors") == 0) {
                    skip_trace = false;
                    lua_pop(L, 2); // key, value
                    break;
                }
                lua_pop(L, 1); // value
            }
        } else if (t == LUA_TBOOLEAN) {
            lua_pushboolean(L, false);
            if (lua_compare(L, -1, -2, LUA_OPEQ))
                skip_trace = true;
            else
                skip_trace = false;
            lua_pop(L, 1); // true
        } else {
            skip_trace = false;
        }
        lua_pop(L, 1); // DEBUG
        if (skip_trace)
            return 1; // original error
    }
    // generate and print trace, then return original error
    luaL_traceback(L, L, NULL, 1);
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    return 1;
}

/// when used as count hook, aborts execution of user function after count instructions
void Tracker::luaTimeoutHook(lua_State *L, lua_Debug *)
{
    luaL_error(L, timeout_error_message);
}

// This functaion can be used internally when the return type is uncertain.
// Use carefully; the return value(s) and error handler will still be on the lua stack
static int RunLuaFunction_inner(lua_State *L, const std::string name, int execLimit)
{
    lua_pushcfunction(L, Tracker::luaErrorHandler);

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
        lua_pop(L, 2); // non-function variable or nil, luaErrorHandler
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
        lua_sethook(L, Tracker::luaTimeoutHook, LUA_MASKCOUNT, execLimit);
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
    
    _providerCountCache.clear();
    _objectCache.clear();
    _accessibilityStale = true;
    _visibilityStale = true;
    for (auto& v : j) {
        if (v.type() != json::value_t::object) {
            fprintf(stderr, "Bad item\n");
            continue; // ignore
        }
        _jsonItems.push_back(JsonItem::FromJSON(v));
        auto& item = _jsonItems.back();
        item.setID(++_lastItemID);
        item.onChange += {this, [this](void* sender) {
            JsonItem* i = (JsonItem*)sender;
            if (!_updatingCache || !_itemChangesDuringCacheUpdate.count(i->getID())) {
                _providerCountCache.clear();
                _accessibilityStale = true;
                _visibilityStale = true;
                if (_updatingCache)
                    _itemChangesDuringCacheUpdate.insert(i->getID());
            } else {
                fprintf(stderr, "WARNING: item toggled multiple times in access rule. Ignoring.\n");
            }
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
            if (_bulkUpdate)
                _bulkItemDisplayUpdates.push_back(i->getID());
            else
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
    
    _providerCountCache.clear();
    _objectCache.clear();
    _sectionRefs.clear();
    _accessibilityStale = true;
    _visibilityStale = true;
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
            for (auto& sec: loc.getSections())
                sec.setParentID(newID);
        }
#endif
        _locations.push_back(std::move(loc)); // TODO: move constructor
        for (auto& sec : _locations.back().getSections()) {
            if (!sec.getRef().empty())
                _sectionNameRefs[sec.getRef()].push_back(sec.getFullID());
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
    int res = 0;
    _isIndirectConnection = false;
    // "codes" starting with $ run Lua functions
    if (!code.empty() && code[0] == '$') {
        res = Tracker::runLuaFunction(_L, code);
        goto done;
    }

    // other codes count items
#ifdef JSONITEM_CI_QUIRK
    {
        const auto lower = JsonItem::toLower(code);
        for (const auto& item : _jsonItems)
        {
            res += item.providesCodeLower(lower);
        }
    }
#else
    for (const auto& item : _jsonItems)
    {
        res += item.providesCode(code);
    }
#endif

    for (const auto& item : _luaItems)
    {
        res += item.providesCode(code);
    }

done:
    if (!_isIndirectConnection)
        _providerCountCache[code] = res;
    return res;
}

Tracker::Object Tracker::FindObjectForCode(const char* code)
{
    const auto it = _objectCache.find(std::string_view(code));
    if (it != _objectCache.end())
        return it->second;
    if (*code == '@') { // location section
        const char *start = code+1;
        const char *t = strrchr(start, '/');
        if (t) { // valid section identifier
            std::string locid = std::string(start, t-start);
            std::string secname = t+1;
            // match by ID (includes all parents)
            auto& loc = getLocation(locid, true);
            for (auto& sec: loc.getSections()) {
                if (sec.getName() != secname)
                    continue;
                _isIndirectConnection = true; // if called during rule resolution, this skips the cache
                return _objectCache.emplace(code, &sec).first->second;
            }
        }
    }
    if (*code == '@') { // location (not section)
        const char *start = code+1;
        auto& loc = getLocation(start, true);
        if (!loc.getID().empty()) {
            _isIndirectConnection = true;
            return _objectCache.emplace(code, &loc).first->second;
        }
    } else {
#ifdef JSONITEM_CI_QUIRK
        const auto lower = JsonItem::toLower(code);
        for (auto& item : _jsonItems) {
            if (item.canProvideCodeLower(lower))
                return _objectCache.emplace(code, &item).first->second;
        }
#else
        for (auto& item : _jsonItems) {
            if (item.canProvideCode(code))
                return _objectCache.emplace(code, &item).first->second;
        }
#endif

        for (auto& item : _luaItems) {
            if (item.canProvideCode(code)) {
                return _objectCache.emplace(code, &item).first->second;
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

template <typename T>
static void eraseDuplicates(std::list<T>& list)
{
    // This keeps the last occurence.
    // FIXME: Complexity is awful.
    auto it = list.begin();
    while (it != list.end()) {
        auto cur = it++;
        if (std::find(it, list.end(), *cur) != list.end())
            list.erase(cur);
    }
}

int Tracker::Lua_Index(lua_State *L, const char* key) {
    if (strcmp(key, "ActiveVariantUID") == 0) {
        lua_pushstring(L, _pack->getVariant().c_str());
        return 1;
    } else if (strcmp(key, "BulkUpdate") == 0) {
        lua_pushboolean(L, _bulkUpdate);
        return 1;
    } else if (strcmp(key, "AllowDefferedLogicUpdate") == 0) {
        lua_pushboolean(L, _allowDeferredLogicUpdate);
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
        if (_bulkUpdate && !val) {
            eraseDuplicates(_bulkItemUpdates);
            for (const auto& id: _bulkItemUpdates)
                onStateChanged.emit(this, id);
            _bulkItemUpdates.clear();
            eraseDuplicates(_bulkItemDisplayUpdates);
            for (const auto& id: _bulkItemDisplayUpdates)
                onDisplayChanged.emit(this, id);
            _bulkItemDisplayUpdates.clear();
            _bulkUpdate = false;
            onBulkUpdateDone.emit(this);
        } else {
            _bulkUpdate = val;
        }
        return true;
    } else if (strcmp(key, "AllowDefferedLogicUpdate") == 0) {
        bool val = lua_isnumber(L, -1) ? (lua_tonumber(L, -1) != 0) : lua_toboolean(L, -1);
        _allowDeferredLogicUpdate = val;
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
#ifdef JSONITEM_CI_QUIRK
    const auto lower = JsonItem::toLower(code);
    for (const auto& item: _jsonItems) {
        if (item.canProvideCodeLower(lower))
            return item;
    }
#else
    for (const auto& item: _jsonItems) {
        if (item.canProvideCode(code))
            return item;
    }
#endif

    for (const auto& item: _luaItems) {
        if (item.canProvideCode(code))
            return item;
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

bool Tracker::isBulkUpdate() const
{
    return _bulkUpdate;
}

bool Tracker::allowDeferredLogicUpdate() const
{
    return _allowDeferredLogicUpdate;
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

AccessibilityLevel Tracker::isReachable(const LocationSection& section)
{
    for (const auto& loc: _locations) {
        if (loc.getID() == section.getParentID()) {
            return isReachable(loc, section);
        }
    }
    return AccessibilityLevel::NONE;
}

bool Tracker::isVisible(const Location::MapLocation& mapLoc)
{
    if (!mapLoc.getInvisibilityRules().empty()
            && resolveRules(mapLoc.getInvisibilityRules(), true) != AccessibilityLevel::NONE)
        return false;
    auto res = resolveRules(mapLoc.getVisibilityRules(), true);
    return (res != AccessibilityLevel::NONE);
}

AccessibilityLevel Tracker::resolveRules(const std::list< std::list<std::string> >& rules, bool visibilityRules)
{
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
            // '^$func' gives direct accessibility level rather than an integer code count
            bool isAccessibilitLevel = s[0] == '^'; // use value as level instead of count
            // '@...' references another location or section
            bool isLocationReference = s[0] == '@';
            // '<rule>:<count>' checks count (e.g. consumables) instead of bool
            int count = 1;
            auto p = s.find(':');
            if (!isAccessibilitLevel && !isLocationReference && p != s.npos) {
                count = atoi(s.c_str()+p+1);
                s = s.substr(0,p);
            }
            if (isAccessibilitLevel) {
                if (s.length() < 3 || s[1] != '$') { // only ^$ supported
                    fprintf(stderr, "Warning: invalid rule \"%s\"",
                            sanitize_print(s).c_str());
                    reachable = AccessibilityLevel::NONE;
                    break;
                }
                s = s.substr(1);
            }
            if (s[0] == '@') {
                const char* start = s.c_str()+1;
                const char* t = strrchr(s.c_str()+1, '/');
                std::string locid = s.substr(1);
                auto& loc = getLocation(locid, true);
                bool match = false;
                AccessibilityLevel sub = AccessibilityLevel::NONE;
                if (!loc.getID().empty()) {
                    // @-Rule for location, not a section
                    if (visibilityRules)
                        sub = isVisible(loc) ? AccessibilityLevel::NORMAL : AccessibilityLevel::NONE;
                    else
                        sub = isReachable(loc);
                    match = true;
                } else if (t) {
                    // @-Rule for a section (also run for missing location with '/')
                    std::string sublocid = locid.substr(0, t-start);
                    std::string subsecname = t+1;
                    auto& subloc = getLocation(sublocid, true);
                    for (auto& subsec: subloc.getSections()) {
                        if (subsec.getName() != subsecname)
                            continue;
                        if (visibilityRules)
                            sub = isVisible(subloc, subsec) ? AccessibilityLevel::NORMAL : AccessibilityLevel::NONE;
                        else
                            sub = isReachable(subloc, subsec);
                        match = true;
                        break;
                    }
                }
                if (match) {
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
                // NOTE: ProvideCountForCode has a cache
                int n = ProviderCountForCode(s);
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
    return glitchedReachable ? AccessibilityLevel::SEQUENCE_BREAK :
           inspectOnlyReachable ? AccessibilityLevel::INSPECT :
               AccessibilityLevel::NONE;
}

AccessibilityLevel Tracker::isReachable(const Location& location, const LocationSection& section)
{
    cacheAccessibility();
    const LocationSection& realSection = section.getRef().empty() ? section : getLocationSection(section.getRef());
    std::string id = realSection.getFullID();
    return _accessibilityCache[id];
}

bool Tracker::isVisible(const Location& location, const LocationSection& section)
{
    const LocationSection& realSection = section.getRef().empty() ? section : getLocationSection(section.getRef());
    std::string id = realSection.getFullID();
    if (realSection.getVisibilityRules().empty())
        return true; // We skip cache for those. Most visibility rules will be empty, so this is a win.
    cacheVisibility();
    return _visibilityCache[id];
}

AccessibilityLevel Tracker::isReachable(const Location& location)
{
    cacheAccessibility();
    return _accessibilityCache[location.getID()];
}

bool Tracker::isVisible(const Location& location)
{
    if (location.getVisibilityRules().empty())
        return true; // We skip cache for those. Most visibility rules will be empty, so this is a win.
    cacheVisibility();
    return _visibilityCache[location.getID()];
}

LuaItem * Tracker::CreateLuaItem()
{
    _luaItems.push_back({});
    _objectCache.clear();
    LuaItem& i = _luaItems.back();
    i.setID(++_lastItemID);
    i.onChange += {this, [this](void* sender) {
        LuaItem* i = (LuaItem*)sender;
        if (!_updatingCache || !_itemChangesDuringCacheUpdate.count(i->getID())) {
            if (!_bulkUpdate)
                _providerCountCache.clear();
            _accessibilityStale = true;
            _visibilityStale = true;
            if (_updatingCache)
                _itemChangesDuringCacheUpdate.insert(i->getID());
        } else {
            fprintf(stderr, "WARNING: item toggled multiple times in access rule. Ignoring.\n");
        }
        if (_bulkUpdate)
            _bulkItemUpdates.push_back(i->getID());
        else
            onStateChanged.emit(this, i->getID());
    }};
    return &i;
}

void Tracker::cacheAccessibility()
{
    if (!_accessibilityStale)
        return;
    _updatingCache = true;
    _accessibilityCache.clear();
    _accessibilityStale = false;

    bool done = false;
    while (!done) {
        done = true;
        for (const auto& location: _locations) {
            auto it = _accessibilityCache.find(location.getID());
            if (it == _accessibilityCache.end() || it->second != AccessibilityLevel::NORMAL) {
                auto res = resolveRules(location.getAccessRules(), false);
                if (it == _accessibilityCache.end()) {
                    _accessibilityCache[location.getID()] = res;
                    done = false;
                }
                else if (it->second != res) {
                    it->second = res;
                    done = false;
                }
            }
            for (const auto& section: location.getSections()) {
                const std::string id = location.getID() + "/" + section.getName();
                it = _accessibilityCache.find(id);
                if (it != _accessibilityCache.end() && it->second == AccessibilityLevel::NORMAL)
                    continue; // nothing to do
                auto res = resolveRules(section.getAccessRules(), false);
                if (it == _accessibilityCache.end()) {
                    _accessibilityCache[id] = res;
                    done = false;
                } else if (it->second != res) {
                    it->second = res;
                    done = false;
                }
            }
        }
    }

    _updatingCache = false;
    _itemChangesDuringCacheUpdate.clear();
}

void Tracker::cacheVisibility()
{
    if (!_visibilityStale)
        return;
    _updatingCache = true;
    _visibilityCache.clear();
    _visibilityStale = false;

    bool done = false;
    while (!done) {
        done = true;
        for (const auto& location: _locations) {
            if (!location.getVisibilityRules().empty()) { // no need to pre-cache empty
                auto it = _visibilityCache.find(location.getID());
                if (it == _visibilityCache.end() || !it->second) {
                    bool res = resolveRules(location.getVisibilityRules(), true) != AccessibilityLevel::NONE;
                    if (it == _visibilityCache.end()) {
                        _visibilityCache[location.getID()] = res;
                        done = false;
                    }
                    else if (it->second != res) {
                        it->second = res;
                        done = false;
                    }
                }
            }
            for (const auto& section: location.getSections()) {
                if (section.getVisibilityRules().empty())
                    continue; // no need to pre-cache
                const std::string id = location.getID() + "/" + section.getName();
                auto it = _visibilityCache.find(id);
                if (it != _visibilityCache.end() && it->second)
                    continue; // nothing to do
                auto res = resolveRules(section.getVisibilityRules(), true) != AccessibilityLevel::NONE;
                if (it == _visibilityCache.end()) {
                    _visibilityCache[id] = res;
                    done = false;
                } else if (it->second != res) {
                    it->second = res;
                    done = false;
                }
            }
        }
    }

    _updatingCache = false;
    _itemChangesDuringCacheUpdate.clear();
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
            std::string id = sec.getFullID();
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
    _providerCountCache.clear();
    _bulkItemUpdates.clear();
    _bulkItemDisplayUpdates.clear();
    _accessibilityStale = true;
    _visibilityStale = true;
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
                    if (key != sec.getFullID())
                        continue;
                    sec.load(it.value());
                }
            }
        }
    }
    eraseDuplicates(_bulkItemUpdates);
    for (const auto& id: _bulkItemUpdates)
        onStateChanged.emit(this, id);
    _bulkItemUpdates.clear();
    eraseDuplicates(_bulkItemDisplayUpdates);
    for (const auto& id: _bulkItemDisplayUpdates)
        onDisplayChanged.emit(this, id);
    _bulkItemDisplayUpdates.clear();
    _bulkUpdate = false;
    onBulkUpdateDone.emit(this);

    return true;
}

void Tracker::setExecLimit(int execLimit)
{
    _execLimit = execLimit;
}

int Tracker::getExecLimit()
{
    return _execLimit;
}
