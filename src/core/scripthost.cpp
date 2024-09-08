#include "scripthost.h"
#include <luaglue/luamethod.h>
#include <luaglue/lua_json.h>
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
    LUA_METHOD(ScriptHost, AddOnFrameHandler, const char*, LuaRef),
    LUA_METHOD(ScriptHost, RemoveOnFrameHandler, const char*),
    LUA_METHOD(ScriptHost, AddOnLocationSectionChangedHandler, const char*, LuaRef),
    LUA_METHOD(ScriptHost, RemoveOnLocationSectionHandler, const char*),
    LUA_METHOD(ScriptHost, RunScriptAsync, const char*, json, LuaRef, LuaRef),
    LUA_METHOD(ScriptHost, RunStringAsync, const char*, json, LuaRef, LuaRef),
};


ScriptHost::ScriptHost(Pack* pack, lua_State *L, Tracker *tracker)
    : _L(L), _pack(pack), _tracker(tracker)
{
    auto flags = _pack->getVariantFlags();
    auto gameFlags = GameInfo::Find(_pack->getGameName()).flags;
    flags.insert(gameFlags.begin(), gameFlags.end());
    _autoTracker = new AutoTracker(_pack->getPlatform(), flags, _pack->getGameName());
    _autoTracker->onStateChange += {this, [this](void*, int index, AutoTracker::State state)
    {
        const auto backend = _autoTracker ? _autoTracker->getName(index) : "";
        // currently started/stopped calls are specific "memory" auto-tracking
        if (backend.empty() || backend == "AP" || backend == "UAT") return;
        if (state == AutoTracker::State::ConsoleConnected) {
            int t = lua_getglobal(_L, "autotracker_started");
            if (t != LUA_TFUNCTION) {
                lua_pop(_L, 1);
            } else if (lua_pcall(_L, 0, 0, 0) != LUA_OK) {
                auto err = lua_tostring(_L, -1);
                fprintf(stderr, "Error running autotracker_started:\n%s\n",
                    err ? err : "Unknown error");
                lua_pop(_L, 1); // error object
            }
        } else if (state == AutoTracker::State::Disabled) {
            int t = lua_getglobal(_L, "autotracker_stopped");
            if (t != LUA_TFUNCTION) {
                lua_pop(_L, 1);
            } else if (lua_pcall(_L, 0, 0, 0) != LUA_OK) {
                auto err = lua_tostring(_L, -1);
                fprintf(stderr, "Error running autotracker_stopped:\n%s\n",
                    err ? err : "Unknown error");
                lua_pop(_L, 1); // error object
            }
            // re-add watches to new backend object
            for (const auto& w: _memoryWatches) {
                _autoTracker->addWatch((unsigned)w.addr, (unsigned)w.len);
            }
        }
    }};
    _autoTracker->onDataChange += {this, [this](void* sender) {
        DEBUG_printf("AutoTracker: Data changed!\n");
        for (size_t i=0; i<_memoryWatches.size(); i++) {
            // NOTE: since watches can change in a callback, we use vector
            auto& w = _memoryWatches[i];
            auto newData = _autoTracker->read((unsigned)w.addr, (unsigned)w.len);
            if (w.data != newData) {
                DEBUG_printf("  %s changed\n", w.name.c_str());
#ifdef DEBUG_TRACKER
                for (size_t i=0; i<(std::min(w.data.size(), newData.size())); i++) {
                    if (w.data[i] != newData[i])
                        DEBUG_printf("%02x: %02x -> %02x\n", (unsigned)i,
                                ((unsigned)w.data[i])&0xff, ((unsigned)newData[i])&0xff);
                }
#endif
                w.data = newData;
                w.dirty = true;
            }
            // NOTE: we run the user callbacks in runMemoryWatchCallbacks
        }
    }};
    _autoTracker->onVariablesChanged += {this, [this](void*, const std::list<std::string>& vars) {
        std::list<std::string> watchVars;
        DEBUG_printf("AutoTracker: Variables changed!\n");
        for (size_t i=0; i<_varWatches.size(); i++) {
            // NOTE: since watches can change in a callback, we use vector
            const auto& pair = _varWatches[i];
            auto name = pair.first;
            for (const auto& varName: vars) {
                if (pair.second.names.find(varName) != pair.second.names.end())
                    watchVars.push_back(varName);
            }
            if (!watchVars.empty()) {
                lua_rawgeti(_L, LUA_REGISTRYINDEX, pair.second.callback);
                _autoTracker->Lua_Push(_L); // arg1: autotracker ("store")
                json j = watchVars;
                json_to_lua(_L, j); // args2: variable names
                if (lua_pcall(_L, 2, 0, 0)) {
                    printf("Error calling Variable Watch Callback for %s: %s\n",
                            pair.first.c_str(), lua_tostring(_L, -1));
                    lua_pop(_L, 1);
                }
            }
            watchVars.clear();
            if (_varWatches.size() <= i) break;
            if (_varWatches[i].first != name) // current item not unchanged
                i--;
        }
    }};
    AutoTracker::Lua_Register(_L);
    _autoTracker->Lua_Push(_L);
    lua_setglobal(_L, "AutoTracker");
    
    _tracker->onStateChanged += { this, [this](void* s, const std::string& id) {
        const auto& item = _tracker->getItemById(id);
        for (size_t i=0; i<_codeWatches.size(); i++) {
            // NOTE: since watches can change in a callback, we use vector
            auto& pair = _codeWatches[i];
            auto name = pair.first;
            bool isWildcard = pair.second.code == "*";
            if (isWildcard || item.canProvideCode(pair.second.code)) {
                printf("Item %s changed, which can provide code \"%s\" for watch \"%s\"\n",
                        id.c_str(), pair.second.code.c_str(), pair.first.c_str());
                lua_rawgeti(_L, LUA_REGISTRYINDEX, pair.second.callback);
                if (isWildcard)
                    lua_pushstring(_L, item.getCodesString().c_str()); // arg1: item code(s)
                else
                    lua_pushstring(_L, pair.second.code.c_str()); // arg1: watched code
                if (lua_pcall(_L, 1, 0, 0)) {
                    printf("Error calling WatchForCode Callback for %s: %s\n",
                            name.c_str(), lua_tostring(_L, -1));
                    lua_pop(_L, 1);
                    return;
                }
            }
            if (_codeWatches.size() <= i) break;
            if (_codeWatches[i].first != name) // current item not unchanged
                i--;
        }
    }};

    _tracker->onLocationSectionChanged += { this, [this](void*, const LocationSection& section) {
        for (size_t i=0; i<_onLocationSectionChangedHandlers.size(); i++) {
            const auto& handler =  _onLocationSectionChangedHandlers[i];
            auto name = handler.name;
            printf("LocationSection %s changed, for watch \"%s\"\n",
                    section.getFullID().c_str(), handler.name.c_str());
            lua_rawgeti(_L, LUA_REGISTRYINDEX, handler.callback);
            section.Lua_Push(_L);
            if (lua_pcall(_L, 1, 0, 0)) {
                printf("Error calling LocationSection handler for %s: %s\n",
                        name.c_str(), lua_tostring(_L, -1));
                lua_pop(_L, 1);
                return;
            }
            if (_onLocationSectionChangedHandlers.size() <= i) // current item removed
                break;
            if (_onLocationSectionChangedHandlers[i].name != name) // current item changed
                --i;
        }
    }};
}

ScriptHost::~ScriptHost()
{
    if (_autoTracker) delete _autoTracker;
    _autoTracker = nullptr;
    _tracker->onStateChanged -= this;
    // NOTE: we leak callback lua refs here
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

    const char* buf = script.c_str();
    size_t len = script.length();
    if (len>=3 && memcmp(buf, "\xEF\xBB\xBF", 3) == 0) {
        fprintf(stderr, "WARNING: skipping BOM of %s\n", sanitize_print(file).c_str());
        buf += 3;
        len -= 3;
    }
    if (luaL_loadbufferx(_L, buf, len, file.c_str(), "t") == LUA_OK) {
        std::string modname = file;
        if (strncasecmp(modname.c_str(), "scripts/", 8) == 0)
            modname = modname.substr(8);
        if (modname.length() > 4 && strcasecmp(modname.c_str() + modname.length() - 4, ".lua") == 0)
            modname = modname.substr(0, modname.length() - 4);
        std::replace(modname.begin(), modname.end(), '/', '.');
        lua_pushstring(_L, modname.c_str());
        if (lua_pcall(_L, 1, 1, 0) == LUA_OK) {
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

std::string ScriptHost::AddMemoryWatch(const std::string& name, unsigned int addr, int len, LuaRef callback, int interval)
{
    if (interval==0) interval=500; /*orig:1000*/ // default
    
    RemoveMemoryWatch(name);
    
    if (!callback.valid()) return ""; // TODO: somehow return nil instead
    if (addr<0 || len<1) {
        luaL_unref(_L, LUA_REGISTRYINDEX, callback.ref);
        return ""; // TODO: somehow return nil instead
    }
    
    // AutoTracker watches are dumb, so we need to keep track of the state
    MemoryWatch w;
    w.callback = callback.ref;
    w.addr = addr;
    w.len = len;
    w.interval = interval;
    w.name = name;
    w.dirty = false;
    
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
        return name;
    } else {
        luaL_unref(_L, LUA_REGISTRYINDEX, w.callback);
        return ""; // TODO: somehoe return nil instead
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

std::string ScriptHost::AddWatchForCode(const std::string& name, const std::string& code, LuaRef callback)
{
    RemoveWatchForCode(name);
    if (code.empty() || !callback.valid()) return ""; // TODO: return nil somehow;
    for (auto it = _codeWatches.begin(); it != _codeWatches.end(); it++) {
        if (it->first == name) {
            _codeWatches.erase(it);
            break;
        }
    }
    _codeWatches.push_back({name, { callback.ref, code }});
    return name;
}

bool ScriptHost::RemoveWatchForCode(const std::string& name)
{
    for (auto it = _codeWatches.begin(); it != _codeWatches.end(); it++) {
        if (it->first == name) {
            luaL_unref(_L, LUA_REGISTRYINDEX, it->second.callback);
            _codeWatches.erase(it);
            break;
        }
    }
    return true;
}

std::string ScriptHost::AddVariableWatch(const std::string& name, const json& variables, LuaRef callback, int)
{
    RemoveVariableWatch(name);
    if (!variables.is_array()) {
        fprintf(stderr, "WARNING: AddVariableWatch: `variables` has to be an array!\n");
        return ""; // TODO: return nil somehow
    }
    if (!callback.valid()) {
        fprintf(stderr, "WARNING: AddVariableWatch: callback is invalid!\n");
        return ""; // TODO: return nil somehow
    }
    std::set<std::string> varnames;
    for (auto& var: variables) if (var.is_string()) varnames.insert(var.get<std::string>());
    if (varnames.empty()) {
        fprintf(stderr, "WARNING: AddVariableWatch: `variables` is empty or invalid!\n");
        return ""; // TODO: return nil somehow
    }
    for (auto it = _varWatches.begin(); it != _varWatches.end(); it++) {
        if (it->first == name) {
            _varWatches.erase(it);
            break;
        }
    }
    _varWatches.push_back({name, { callback.ref, varnames }});
    return name;
}

bool ScriptHost::RemoveVariableWatch(const std::string& name)
{
    for (auto it = _varWatches.begin(); it != _varWatches.end(); it++) {
        if (it->first == name) {
            luaL_unref(_L, LUA_REGISTRYINDEX, it->second.callback);
            _varWatches.erase(it);
            break;
        }
    }
    return true;
}

std::string ScriptHost::AddOnFrameHandler(const std::string& name, LuaRef callback)
{
    RemoveOnFrameHandler(name);
    if (!callback.valid())
        luaL_error(_L, "Invalid callback");
    _onFrameHandlers.push_back(OnFrameHandler{callback.ref, name, Ui::getMicroTicks()});
    return name;
}

bool ScriptHost::RemoveOnFrameHandler(const std::string& name)
{
    for (auto it = _onFrameHandlers.begin(); it != _onFrameHandlers.end(); it++) {
        if (it->name == name) {
            luaL_unref(_L, LUA_REGISTRYINDEX, it->callback);
            _onFrameHandlers.erase(it);
            return true;
        }
    }
    return false;
}

const std::string& ScriptHost::AddOnLocationSectionChangedHandler(const std::string& name, LuaRef callback)
{
    RemoveOnLocationSectionHandler(name);
    if (!callback.valid())
        luaL_error(_L, "Invalid callback");
    _onLocationSectionChangedHandlers.push_back({callback.ref, name});
    return _onLocationSectionChangedHandlers.back().name;
}

bool ScriptHost::RemoveOnLocationSectionHandler(const std::string &name)
{
    for (auto it = _onLocationSectionChangedHandlers.begin(); it != _onLocationSectionChangedHandlers.end(); ++it) {
        if (it->name == name) {
            luaL_unref(_L, LUA_REGISTRYINDEX, it->callback);
            _onLocationSectionChangedHandlers.erase(it);
            return true;
        }
    }
    return false;
}

json ScriptHost::RunScriptAsync(const std::string& file, const json& arg, LuaRef completeCallback, LuaRef progressCallback)
{
    std::string script;
    if (!_tracker || !_tracker->getPack() || !_tracker->getPack()->ReadFile(file, script)) {
        luaL_error(_L, "Could not load script!");
    }
    return runAsync(file, script, arg, completeCallback, progressCallback);
}

json ScriptHost::RunStringAsync(const std::string& script, const json& arg, LuaRef completeCallback, LuaRef progressCallback)
{
    return runAsync("", script, arg, completeCallback, progressCallback);
}

json ScriptHost::runAsync(const std::string& name, const std::string& script, const json& arg, LuaRef completeCallback, LuaRef progressCallback)
{
    // if progress callback is nil, we free the ref and store a NOREF instead to avoid  unnecessary locking on onFrame
    lua_rawgeti(_L, LUA_REGISTRYINDEX, progressCallback.ref);
    if (lua_isnil(_L, -1)) {
        luaL_unref(_L, LUA_REGISTRYINDEX, progressCallback.ref);
        progressCallback.ref = LUA_NOREF;
    }
    lua_pop(_L, 1);
    try {
        _asyncTasks.emplace_back(
            name, script, arg, completeCallback, progressCallback, _tracker->getPack()
        );
    } catch (std::exception& e) {
        luaL_unref(_L, LUA_REGISTRYINDEX, completeCallback.ref);
        throw e;
    }
    return json::object();
}

bool ScriptHost::onFrame()
{
    bool res = autoTrack();

    auto it = _asyncTasks.begin();
    while (it != _asyncTasks.end()) {
        auto& task = *it;
        bool running = task.running();
        {
            auto ref = task.getProgressCallbackRef();
            if (ref != LUA_NOREF) {
                json j;
                while (task.getProgress(j)) {
                    lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
                    json_to_lua(_L, j);
                    if (lua_pcall(_L, 1, 0, 0)) {
                        printf("Error calling callback for async: %s\n", lua_tostring(_L, -1));
                        lua_pop(_L, 1);
                    }
                }
            }
        }
        if (!running) {
            std::string msg;
            auto ref = task.getCompleteCallbackRef();
            if (task.error(msg)) {
                fprintf(stderr, "Async error: %s\n", msg.c_str());
            } else {
                lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
                json_to_lua(_L, task.getResult());
                if (lua_pcall(_L, 1, 0, 0)) {
                    printf("Error calling callback for async: %s\n", lua_tostring(_L, -1));
                    lua_pop(_L, 1);
                }
            }
            luaL_unref(_L, LUA_REGISTRYINDEX, ref);
            auto progressRef = task.getProgressCallbackRef();
            if (progressRef != LUA_NOREF) {
                luaL_unref(_L, LUA_REGISTRYINDEX, progressRef);
            }
            _asyncTasks.erase(it++);
        } else {
            it++;
        }
    }

    for (size_t i=0; i<_onFrameHandlers.size(); i++) {
        auto name = _onFrameHandlers[i].name;
        auto now = Ui::getMicroTicks();
        auto elapsedUs = now - _onFrameHandlers[i].lastTimestamp;
        double elapsed = (double)elapsedUs / 1000000.0;
        _onFrameHandlers[i].lastTimestamp = now;

        // For now we use the same exec limit as in Tracker, which is 3-4ms for pure Lua on a fast PC.
        runLuaFunction(_onFrameHandlers[i].callback, name, [elapsed](lua_State *L) {
            Lua(L).Push(elapsed);
            return 1;
        }, Tracker::getExecLimit());

        if (_onFrameHandlers.size() <= i)
            break; // callback does not exist anymore
        if (_onFrameHandlers[i].name != name)
            i--; // callback was modified in callback
    }

    return res;
}

bool ScriptHost::autoTrack()
{
    if (_autoTracker && _autoTracker->doStuff()) {
        // autotracker changed the cache

        // run callbacks
        runMemoryWatchCallbacks();

        return true;
    }

    // autotracker didn't change anything
    return false;
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

void ScriptHost::runMemoryWatchCallbacks()
{
    if (!_autoTracker || !_autoTracker->isAnyConnected())
        return;

    // we need to run callbacks because the autotracker changed some cache
    // but really we only need to run the ones that are marked dirty
    for (size_t i = 0; i < _memoryWatches.size(); i++) {
        // NOTE: since watches can change in a callback, we use vector
        auto& w = _memoryWatches[i];

        // skip this watch if it's not dirty
        if (!w.dirty)
            continue;

        auto name = w.name;

        // run memory watch callback
        bool res = true; // on error: don't rerun
        runLuaFunction(w.callback, "Memory Watch Callback for " + name, res, [this](lua_State* L){
            _autoTracker->Lua_Push(L); // arg1: autotracker ("segment")
            return 1;
        });

        if (_memoryWatches.size() <= i)
            break; // watch does not exist anymore
        if (_memoryWatches[i].name != name)
            i--; // watch at i changed (was removed)
        else if (res != false)
            _memoryWatches[i].dirty = false; // watch returned non-false
    }
}


ScriptHost::ThreadContext::ThreadContext(const std::string& name, const std::string& script, const json& arg, LuaRef completeCallback, LuaRef progressCallback, const Pack* pack)
        : _state(State::Running), _completeCallback(completeCallback), _progressCallback(progressCallback), _luaio(pack), _stop(false)
{
    printf("Starting Thread...\n");
    _L = luaL_newstate();
    if (!_L) {
        fprintf(stderr, "Error creating Lua State!\n");
        _state = State::Error;
        throw std::runtime_error("Error creating ThreadContext Lua State");
    }
    // TODO: merge with poptracker.cpp
    std::initializer_list<const luaL_Reg> luaLibs = {
      {LUA_GNAME, luaopen_base},
      {LUA_TABLIBNAME, luaopen_table},
      {LUA_OSLIBNAME, luaopen_os}, // this has to be reduced in functionality
      {LUA_STRLIBNAME, luaopen_string},
      {LUA_MATHLIBNAME, luaopen_math},
      {LUA_UTF8LIBNAME, luaopen_utf8},
    };
    for (const auto& lib: luaLibs) {
        luaL_requiref(_L, lib.name, lib.func, 1);
        lua_pop(_L, 1);
    }
    // load lua debugger if enabled during compile AND run time
#ifdef WITH_LUA_DEBUG
    if (_config.value<bool>("lua_debug", false)) {
        luaL_requiref(_L, LUA_DBLIBNAME, luaopen_debug, 1);
        lua_pop(_L, 1);
    }
#endif
    // block some global function until we decide what to allow
    for (const auto& blocked: { "load", "loadfile", "loadstring" }) {
        lua_pushnil(_L);
        lua_setglobal(_L, blocked);
    }
    // implement require
    lua_pushcfunction(_L, luasandbox_require);
    lua_setglobal(_L, "require");
    // reduce os
    lua_getglobal(_L, LUA_OSLIBNAME); // get full os
    lua_createtable(_L, 0, 3); // create new os
    for (const auto& field : { "clock", "date", "difftime", "time" }) {
        lua_getfield(_L, -2, field);
        lua_setfield(_L, -2, field);
    }
    lua_setglobal(_L, LUA_OSLIBNAME); // store new os, deref old os
    // custom IO
    LuaPackIO::Lua_Register(_L);
    LuaPackIO::File::Lua_Register(_L);
    _luaio.Lua_Push(_L);
    lua_setglobal(_L, LUA_IOLIBNAME);
    // "fake" ScriptHost for async context
    _scriptHost.reset(new AsyncScriptHost(this));
    AsyncScriptHost::Lua_Register(_L);
    _scriptHost->Lua_Push(_L);
    lua_setglobal(_L, "ScriptHost");
    // store pack in registry for "private" use (in require)
    lua_pushstring(_L, "Pack");
    lua_pushlightuserdata(_L, (void*)pack);
    lua_settable(_L, LUA_REGISTRYINDEX);
    // arg
    json_to_lua(_L, arg);
    lua_setglobal(_L, "arg");

    _thread = std::thread(
        [this, script, name]() {
            static const char this_key = 'k';

            // hook to be able to check the _stop flag
            auto hook = [](lua_State *L, lua_Debug *ar) {
                lua_pushlightuserdata(L, (void *)&this_key); // static address value as key
                lua_gettable(L, LUA_REGISTRYINDEX);  // retrieve stored this
                ScriptHost::ThreadContext* self = static_cast<ScriptHost::ThreadContext*>(
                    lua_touserdata(L, -1)
                );
                lua_pop(L, 1);
                if (self->_stop)
                    luaL_error(L, "Pack unloaded");
            };

            // store this into Lua state
            lua_pushlightuserdata(_L, (void *)&this_key); // static address value as key
            lua_pushlightuserdata(_L, (void *)this); // this as value
            lua_settable(_L, LUA_REGISTRYINDEX);

            // set hook to be able to abort execution when closing pack
            lua_sethook(_L, hook, LUA_MASKCOUNT, 500000);

            // TODO: merge with LoadScript
            const char* buf = script.c_str();
            size_t len = script.length();
            if (len>=3 && memcmp(buf, "\xEF\xBB\xBF", 3) == 0) {
                fprintf(stderr, "WARNING: skipping BOM of %s\n", sanitize_print(name).c_str());
                buf += 3;
                len -= 3;
            }
            if (luaL_loadbufferx(_L, buf, len, name.c_str(), "t") == LUA_OK) {
                std::string modname = name;
                if (strncasecmp(modname.c_str(), "scripts/", 8) == 0)
                    modname = modname.substr(8);
                if (modname.length() > 4 && strcasecmp(modname.c_str() + modname.length() - 4, ".lua") == 0)
                    modname = modname.substr(0, modname.length() - 4);
                std::replace(modname.begin(), modname.end(), '/', '.');
                lua_pushstring(_L, modname.c_str());
                if (lua_pcall(_L, 1, 1, 0) == LUA_OK) {
                    // if it was executed successfully, pop everything from stack
                    _result = lua_to_json(_L);
                    lua_pop(_L, lua_gettop(_L)); // TODO: lua_settop(L, 0); ?
                    _state = State::Done;
                } else {
                    _errorMessage = "Error running script async: ";
                    _errorMessage += lua_tostring(_L, -1);
                    lua_pop(_L, lua_gettop(_L)); // TODO: lua_settop(L, 0); ?
                    _state = State::Error;
                }
            } else {
                _errorMessage = "Error running script async: ";
                _errorMessage += lua_tostring(_L, -1);
                lua_pop(_L, 1);
                _state = State::Error;
            }
            printf("Thread Done\n");
        }
    );
}

ScriptHost::ThreadContext::~ThreadContext() {
    _stop = true;
    if (_thread.joinable())
        _thread.join();
    lua_close(_L);
}

bool ScriptHost::ThreadContext::running()
{
    return _state == State::Running;
}

bool ScriptHost::ThreadContext::error(std::string& message)
{
    message = _errorMessage;
    return _state == State::Error;
}

int ScriptHost::ThreadContext::getCompleteCallbackRef() const
{
    return _completeCallback.ref;
}

int ScriptHost::ThreadContext::getProgressCallbackRef() const
{
    return _progressCallback.ref;
}

const json& ScriptHost::ThreadContext::getResult() const
{
    return _result;
}

bool ScriptHost::ThreadContext::getProgress(json& progress)
{
    std::lock_guard<std::mutex> lock(_progressMutex);
    if (_progressData.empty())
        return false;
    progress = _progressData.front();
    _progressData.pop();
    return true;
}

void ScriptHost::ThreadContext::addProgress(const json& progress)
{
    std::lock_guard<std::mutex> lock(_progressMutex);
    _progressData.push(progress);
}

void ScriptHost::ThreadContext::addProgress(json&& progress)
{
    std::lock_guard<std::mutex> lock(_progressMutex);
    _progressData.push(progress);
}


const LuaInterface<AsyncScriptHost>::MethodMap AsyncScriptHost::Lua_Methods = {
    LUA_METHOD(AsyncScriptHost, AsyncProgress, json),
};

AsyncScriptHost::AsyncScriptHost(ScriptHost::ThreadContext* context)
    : _context(context)
{
}

void AsyncScriptHost::AsyncProgress(const json& progress)
{
    _context->addProgress(progress);
}
