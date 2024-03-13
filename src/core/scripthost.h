#ifndef _CORE_SCRIPTHOST_H
#define _CORE_SCRIPTHOST_H

#include <luaglue/luainterface.h>
#include <luaglue/lua_json.h>
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


class ScriptHost;

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
        LuaRef _callback;
        LuaPackIO _luaio;
        json _result;
        bool _stop;
        std::string _errorMessage;

    public:
        ThreadContext(const ThreadContext&) = delete;
        ThreadContext(const std::string& name, const std::string& script, const json& arg, LuaRef callback,
                      const Pack* pack);
        virtual ~ThreadContext();

        bool running();
        bool error(std::string& message);
        int getCallbackRef() const;
        const json& getResult() const;
    };

    bool LoadScript(const std::string& file);
    LuaItem *CreateLuaItem();
    std::string AddMemoryWatch(const std::string& name, unsigned int addr, int len, LuaRef callback, int interval);
    bool RemoveMemoryWatch(const std::string& name);
    std::string AddWatchForCode(const std::string& name, const std::string& code, LuaRef callback);
    bool RemoveWatchForCode(const std::string& name);
    std::string AddVariableWatch(const std::string& name, const json& variables, LuaRef callback, int interval);
    bool RemoveVariableWatch(const std::string& name);
    json RunScriptAsync(const std::string& file, const json& arg, LuaRef callback);
    json RunStringAsync(const std::string& script, const json& arg, LuaRef callback);
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
    AutoTracker *_autoTracker = nullptr;
    std::list<ThreadContext> _asyncTasks;

private:
    // This will be called every frame to run auto-tracking
    bool autoTrack();
    json runAsync(const std::string& name, const std::string& script, const json& arg, LuaRef callback);

protected: // Lua interface implementation
    static constexpr const char Lua_Name[] = "ScriptHost";
    static const LuaInterface::MethodMap Lua_Methods;
};


#endif // _CORE_SCRIPTHOST_H
