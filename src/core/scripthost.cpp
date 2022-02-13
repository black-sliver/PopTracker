#include "scripthost.h"
#include "../luaglue/luamethod.h"
#include <stdio.h>
#include "gameinfo.h"


#ifdef DEBUG_TRACKER
#define DEBUG_printf printf
#else
#define DEBUG_printf(...)
#endif


const LuaInterface<ScriptHost>::MethodMap ScriptHost::Lua_Methods = {
    LUA_METHOD(ScriptHost, LoadScript, const char*),
    LUA_METHOD(ScriptHost, AddMemoryWatch, const char*, int, int, LuaRef, int),
    LUA_METHOD(ScriptHost, RemoveMemoryWatch, const char*),
    LUA_METHOD(ScriptHost, AddWatchForCode, const char*, const char*, LuaRef),
    LUA_METHOD(ScriptHost, RemoveWatchForCode, const char*),
    LUA_METHOD(ScriptHost, CreateLuaItem, void),
    LUA_METHOD(ScriptHost, AddVariableWatch, const char*, json, LuaRef, int),
    LUA_METHOD(ScriptHost, RemoveVariableWatch, const char*),
};


ScriptHost::ScriptHost(Pack* pack, lua_State *L, Tracker *tracker)
    : _L(L), _pack(pack), _tracker(tracker)
{
    auto flags = _pack->getVariantFlags();
    auto gameFlags = GameInfo::Find(_pack->getGameName()).flags;
    flags.insert(gameFlags.begin(), gameFlags.end());
    _autoTracker = new AutoTracker(_pack->getPlatform(), flags);
    _autoTracker->onDataChange += {this, [this](void* sender) {
        DEBUG_printf("AutoTracker: Data changed!\n");
        for (auto& w : _memoryWatches) {
            auto newData = _autoTracker->read((unsigned)w.addr, (unsigned)w.len);
            if (w.data != newData) {
                DEBUG_printf("  %s changed\n", w.name.c_str());
                w.data = newData;
                lua_rawgeti(_L, LUA_REGISTRYINDEX, w.callback);
                _autoTracker->Lua_Push(_L); // arg1: autotracker ("segment")
                if (lua_pcall(_L, 1, 0, 0)) {
                    printf("Error calling Memory Watch Callback for %s: %s\n",
                            w.name.c_str(), lua_tostring(_L, -1));
                    // TODO: clean up lua stack
                }
            }
        }
    }};
    _autoTracker->onVariablesChanged += {this, [this](void*, const std::list<std::string>& vars) {
        std::list<std::string> watchVars;
        DEBUG_printf("AutoTracker: Variables changed!\n");
        for (const auto& it: _varWatches) {
            for (const auto& varName: vars) {
                if (it.second.names.find(varName) != it.second.names.end())
                    watchVars.push_back(varName);
            }
            if (!watchVars.empty()) {
                lua_rawgeti(_L, LUA_REGISTRYINDEX, it.second.callback);
                _autoTracker->Lua_Push(_L); // arg1: autotracker ("segment")
                json j = watchVars;
                json_to_lua(_L, j); // args2: variable names
                if (lua_pcall(_L, 2, 0, 0)) {
                    printf("Error calling Variable Watch Callback for %s: %s\n",
                            it.first.c_str(), lua_tostring(_L, -1));
                    // TODO: clean up lua stack
                }
            }
            watchVars.clear();
        }
    }};
    AutoTracker::Lua_Register(_L);
    _autoTracker->Lua_Push(_L);
    lua_setglobal(_L, "AutoTracker");
    
    _tracker->onStateChanged += { this, [this](void* s, const std::string& id) {
        const auto& item = _tracker->getItemById(id);
        for (auto& pair: _codeWatches) {
            if (item.canProvideCode(pair.second.code)) {
                printf("Item %s changed, which can provide code \"%s\" for watch \"%s\"\n",
                        id.c_str(), pair.second.code.c_str(), pair.first.c_str());
                lua_rawgeti(_L, LUA_REGISTRYINDEX, pair.second.callback);
                lua_pushstring(_L, pair.second.code.c_str()); // arg1: code
                if (lua_pcall(_L, 1, 0, 0)) {
                    printf("Error calling Memory Watch Callback for %s: %s\n",
                            pair.first.c_str(), lua_tostring(_L, -1));
                    // TODO: clean up lua stack
                    return;
                }
            }
        }
    }};
}

ScriptHost::~ScriptHost()
{
    if (_autoTracker) delete _autoTracker;
    _autoTracker = nullptr;
    _tracker->onStateChanged -= this;
}

bool ScriptHost::LoadScript(const std::string& file) 
{
    std::string script;
    printf("Loading \"%s\" ...\n", file.c_str());
    if (!_pack->ReadFile(file, script)) {
        // TODO; throw exception?
        fprintf(stderr, "File not found!\n");
        return false;
    }
    
    if (luaL_loadbuffer(_L, script.c_str(), script.length(), file.c_str()) == LUA_OK) {
        if (lua_pcall(_L, 0, 1, 0) == LUA_OK) {
            // if it was executed successfully, pop everything from stack
            lua_pop(_L, lua_gettop(_L)); // TODO: lua_settop(L, 0); ? 
        } else {
            // TODO: throw exception?
            const char* msg = lua_tostring(_L,-1);
            printf("Error running \"%s\": %s!\n", file.c_str(), msg);
            lua_pop(_L, lua_gettop(_L)); // TODO: lua_settop(L, 0); ? 
            return false;
        }
    } else {
        fprintf(stderr, "Error loading \"%s\": %s!\n", file.c_str(), lua_tostring(_L,-1));
        lua_pop(_L, 1); // TODO: verify this is correct
        return false;
    }
    
    return true;
}

LuaItem* ScriptHost::CreateLuaItem()
{
    // FIXME: keep track of items
    return _tracker->CreateLuaItem();
}

bool ScriptHost::AddMemoryWatch(const std::string& name, int addr, int len, LuaRef callback, int interval)
{
    if (interval==0) interval=500; /*orig:1000*/ // default
    
    RemoveMemoryWatch(name);
    
    if (!callback.valid()) return false;
    if (addr<0 || len<1) {
        luaL_unref(_L, LUA_REGISTRYINDEX, callback.ref);
        return false;
    }
    
    // AutoTracker watches are dumb, so we need to keep track of the state
    MemoryWatch w;
    w.callback = callback.ref;
    w.addr = addr;
    w.len = len;
    w.interval = interval;
    w.name = name;
    
    if (_autoTracker->addWatch((unsigned)w.addr, (unsigned)w.len)) {
        printf("Added watch %s for <0x%06x,0x%02x>\n",
                w.name.c_str(), (unsigned)w.addr, (unsigned)w.len);
        bool updateInterval = true;
        for (auto& other : _memoryWatches) {
            if (other.interval <= w.interval) {
                updateInterval = false;
                break;
            }
        }
        if (updateInterval) _autoTracker->setInterval(w.interval);
        _memoryWatches.push_back(w);
        return true;
    } else {
        luaL_unref(_L, LUA_REGISTRYINDEX, w.callback);
        return false;
    }
}

bool ScriptHost::RemoveMemoryWatch(const std::string& name)
{
    // TODO: use a map instead?
    for (auto it=_memoryWatches.begin(); it!=_memoryWatches.end(); it++) {
        if (it->name == name) {
            bool split = false;
            auto name = it->name;
            auto addr = it->addr;
            auto len = it->len;
            luaL_unref(_L, LUA_REGISTRYINDEX, it->callback);
            _memoryWatches.erase(it);
            // NOTE: AutoTracker::RemoveWatch takes a simple byte range at the moment
            for (const auto& other: _memoryWatches) {
                if (other.addr<=addr && other.addr+other.len>=addr+len) {
                    // entire range still being watched
                    split = false;
                    len = 0;
                    break;
                } else if (other.addr<=addr && other.addr+other.len>addr) {
                    // start still being watched
                    size_t n = other.addr + other.len - addr;
                    addr += n;
                    len -= n;
                    if (!len) break;
                } else if (other.addr<addr+len && other.addr+other.len>=addr+len) {
                    // end still being watched
                    len = other.addr-addr;
                    if (!len) break;
                } else if (other.addr>addr && other.addr+other.len<addr+len) {
                    // some range in the middle still being watched, not implemented
                    split = true;
                    len = 0;
                }
            }
            if (split) {
                fprintf(stderr, "WARNING: RemoveWatch: splitting existing byte ranges not implemented\n");
            }
            if (len) {
                _autoTracker->removeWatch(addr,len);
                printf("Removed watch %s, range <0x%06x,0x%02x>\n",
                        name.c_str(), (unsigned)addr, (unsigned)len);
            } else {
                printf("Removed watch %s\n", name.c_str());
            }
            return true;
        }
    }
    return false;
}

bool ScriptHost::AddWatchForCode(const std::string& name, const std::string& code, LuaRef callback)
{
    RemoveWatchForCode(name);
    if (code.empty() || !callback.valid()) return false;
    _codeWatches[name] = { callback.ref, code };
    return true;
}

bool ScriptHost::RemoveWatchForCode(const std::string& name)
{
    auto it = _codeWatches.find(name);
    if (it == _codeWatches.end()) return false;
    luaL_unref(_L, LUA_REGISTRYINDEX, it->second.callback);
    _codeWatches.erase(it);
    return true;
}

bool ScriptHost::AddVariableWatch(const std::string& name, const json& variables, LuaRef callback, int)
{
    RemoveVariableWatch(name);
    if (!variables.is_array()) {
        fprintf(stderr, "WARNING: AddVariableWatch: `variables` has to be an array!\n");
        return false;
    }
    if (!callback.valid()) {
        fprintf(stderr, "WARNING: AddVariableWatch: callback is invalid!\n");
        return false;
    }
    std::set<std::string> varnames;
    for (auto& var: variables) if (var.is_string()) varnames.insert(var.get<std::string>());
    if (varnames.empty()) {
        fprintf(stderr, "WARNING: AddVariableWatch: `variables` is empty or invalid!\n");
        return false;
    }
    _varWatches[name] = { callback.ref, varnames };
    return true;
}

bool ScriptHost::RemoveVariableWatch(const std::string& name)
{
    auto it = _varWatches.find(name);
    if (it == _varWatches.end()) return false;
    luaL_unref(_L, LUA_REGISTRYINDEX, it->second.callback);
    _varWatches.erase(it);
    return true;
}

void ScriptHost::resetWatches()
{
    bool changed = false;
    for (auto& watch : _memoryWatches) {
        if (watch.data.empty()) continue;
        changed = true;
        watch.data.clear();
    }
    _autoTracker->clearCache();
    if (changed && _autoTracker) {
        _autoTracker->onDataChange.emit(_autoTracker);
    }
}

bool ScriptHost::autoTrack()
{
    // This is called every frame to run auto-tracking
    // returns true if auto-tracking changed stuff, false otherwise
    if (_autoTracker) return _autoTracker->doStuff();
    return false;
}
