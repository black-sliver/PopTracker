#ifndef _CORE_SCRIPTHOST_H
#define _CORE_SCRIPTHOST_H

#include <luaglue/luainterface.h>
#include <luaglue/lua_json.h>
#include <luaglue/luapp.h>
#include "pack.h"
#include "autotracker.h"
#include "tracker.h"
#include "luaitem.h"
#include <string>
#include <vector> // TODO: replace by new[] uint8_t?

class ScriptHost;

class ScriptHost : public LuaInterface<ScriptHost> {
    friend class LuaInterface;
    
public:
    ScriptHost(Pack *pack, lua_State *L, Tracker* tracker);
    virtual ~ScriptHost();
    
    bool LoadScript(const std::string& file);
    LuaItem *CreateLuaItem();
    std::string AddMemoryWatch(const std::string& name, unsigned int addr, int len, LuaRef callback, int interval);
    bool RemoveMemoryWatch(const std::string& name);
    std::string AddWatchForCode(const std::string& name, const std::string& code, LuaRef callback);
    bool RemoveWatchForCode(const std::string& name);
    std::string AddVariableWatch(const std::string& name, const json& variables, LuaRef callback, int interval);
    bool RemoveVariableWatch(const std::string& name);
    std::string AddOnFrameHandler(const std::string& name, LuaRef callback);
    bool RemoveOnFrameHandler(const std::string& name);
    void resetWatches();
    
    // This is called every frame. Returns true if state was changed by auto-tracking.
    bool onFrame();

    void runMemoryWatchCallbacks();

    AutoTracker* getAutoTracker() { return _autoTracker; }
    
    struct MemoryWatch
    {
        int callback;
        unsigned int addr;
        int len;
        int interval;
        std::string name;
        std::vector<uint8_t> data;
        bool dirty; // flag is set when data under watch is changed. flag is only cleared when the callback does not return false
    };
    struct CodeWatch
    {
        int callback;
        std::string code;
    };
    struct VarWatch
    {
        int callback;
        std::set<std::string> names;
    };
    
protected:
    lua_State *_L;
    Pack *_pack;
    Tracker *_tracker;
    std::vector<MemoryWatch> _memoryWatches;
    std::vector<std::pair<std::string, CodeWatch> > _codeWatches;
    std::vector<std::pair<std::string, VarWatch> > _varWatches;
    std::vector<std::pair<std::string, LuaRef> > _onFrameCallbacks;
    AutoTracker *_autoTracker = nullptr;

private:
    // This will be called every frame to run auto-tracking
    bool autoTrack();
    // Run a Lua function defined in ref, return bool its result as boolean.
    // ArgsHook can push arguments to the stack and return the number of pushed arguments.
    template <class T>
    int runLuaFunction(int ref, const std::string& name, T& res, std::function<int(lua_State*)> argsHook=nullptr, int execLimit=0)
    {
        lua_pushcfunction(_L, Tracker::luaErrorHandler);
        lua_rawgeti(_L, LUA_REGISTRYINDEX, ref);
        int nargs = argsHook ? argsHook(_L) : 0;
        if (execLimit > 0)
            lua_sethook(_L, Tracker::luaTimeoutHook, LUA_MASKCOUNT, execLimit);
        int status = lua_pcall(_L, nargs, 1, -nargs-2);
        if (execLimit > 0)
            lua_sethook(_L, nullptr, 0, 0);

        if (status) {
            auto err = lua_tostring(_L, -1);
            if (err)
                printf("Error calling %s: %s\n", name.c_str(), err);
            else
                printf("Error calling %s (%d)\n", name.c_str(), status);
            lua_pop(_L, 2); // error + error func
            return status;
        } else {
            try {
                res = Lua(_L).Get<T>(-1);
            } catch (std::invalid_argument&) {
                lua_pop(_L, 2); // result + error func
                return -1;
            }
            lua_pop(_L, 2); // result + error func
            return LUA_OK;
        }
    }

    int runLuaFunction(int ref, const std::string& name, std::function<int(lua_State*)> argsHook=nullptr, int execLimit=0)
    {
        bool ignore;
        return runLuaFunction<bool>(ref, name, ignore, argsHook, execLimit);
    }

protected: // Lua interface implementation
    static constexpr const char Lua_Name[] = "ScriptHost";
    static const LuaInterface::MethodMap Lua_Methods;
};


#endif // _CORE_SCRIPTHOST_H
