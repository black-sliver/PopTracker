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
#include <vector>
#include <list>
#include <thread>
#include "../luasandbox/luapackio.h"
#include "../luasandbox/require.h"
#include "../uilib/timer.h"


class ScriptHost;
class AsyncScriptHost;


class ScriptHost : public LuaInterface<ScriptHost> {
    friend class LuaInterface;
    
public:
    ScriptHost(Pack *pack, lua_State *L, Tracker* tracker);
    virtual ~ScriptHost();

    class ThreadContext {
    public:
        enum class State {
            Running = 0,
            Done = 1,
            Error = -1,
        };

    private:
        State _state;
        lua_State* _L;
        std::thread _thread;
        LuaRef _completeCallback;
        LuaRef _progressCallback;
        LuaPackIO _luaio;
        json _result;
        bool _stop;
        std::string _errorMessage;
        std::queue<json> _progressData;
        std::mutex _progressMutex;
        std::unique_ptr<AsyncScriptHost> _scriptHost;  // has to be pointer unless we pull out ThreadContext and reorder

    public:
        ThreadContext(const ThreadContext&) = delete;
        ThreadContext(const std::string& name, const std::string& script, const json& arg,
                      LuaRef completeCallback, LuaRef progressCallback,
                      const Pack* pack);
        virtual ~ThreadContext();

        bool running();
        bool error(std::string& message);
        int getCompleteCallbackRef() const;
        int getProgressCallbackRef() const;
        const json& getResult() const;
        bool getProgress(json& progress);
        void addProgress(const json& progress);
        void addProgress(json&& progress);
    };

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
    json RunScriptAsync(const std::string& file, const json& arg, LuaRef completeCallback, LuaRef progressCallback);
    json RunStringAsync(const std::string& script, const json& arg, LuaRef completeCallback, LuaRef progressCallback);
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

    struct OnFrameHandler
    {
        int callback;
        std::string name;
        Ui::microtick_t lastTimestamp;
    };

protected:
    lua_State *_L;
    Pack *_pack;
    Tracker *_tracker;
    std::vector<MemoryWatch> _memoryWatches;
    std::vector<std::pair<std::string, CodeWatch> > _codeWatches;
    std::vector<std::pair<std::string, VarWatch> > _varWatches;
    std::vector<OnFrameHandler> _onFrameHandlers;
    AutoTracker *_autoTracker = nullptr;
    std::list<ThreadContext> _asyncTasks;

private:
    // This will be called every frame to run auto-tracking
    bool autoTrack();
    json runAsync(const std::string& name, const std::string& script, const json& arg, LuaRef completeCallback, LuaRef progressCallback);
    // Run a Lua function defined in ref, return its result as boolean.
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


class AsyncScriptHost : public LuaInterface<AsyncScriptHost> {
    friend class LuaInterface;

public:
    AsyncScriptHost(ScriptHost::ThreadContext* context);

    void AsyncProgress(const json& progress);

private:
    ScriptHost::ThreadContext *_context;

protected: // Lua interface implementation
    static constexpr const char Lua_Name[] = "ScriptHost";
    static const LuaInterface::MethodMap Lua_Methods;
};


#endif // _CORE_SCRIPTHOST_H
