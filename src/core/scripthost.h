#ifndef _CORE_SCRIPTHOST_H
#define _CORE_SCRIPTHOST_H

#include "../luaglue/luainterface.h"
#include "../luaglue/lua_json.h"
#include "pack.h"
#include "autotracker.h"
#include "tracker.h"
#include "luaitem.h"
#include <string>
#include <list>
#include <vector> // TODO: replace by new[] uint8_t?

class ScriptHost;

class ScriptHost : public LuaInterface<ScriptHost> {
    friend class LuaInterface;
    
public:
    ScriptHost(Pack *pack, lua_State *L, Tracker* tracker);
    virtual ~ScriptHost();
    
    bool LoadScript(const std::string& file);
    LuaItem *CreateLuaItem();
    bool AddMemoryWatch(const std::string& name, int addr, int len, LuaRef callback, int interval);
    bool RemoveMemoryWatch(LuaVariant name);
    bool AddWatchForCode(const std::string& name, const std::string& code, LuaRef callback);
    bool RemoveWatchForCode(const std::string& name);
    bool AddVariableWatch(const std::string& name, const json& variables, LuaRef callback, int interval);
    bool RemoveVariableWatch(const std::string& name);
    void resetWatches();
    
    bool autoTrack();
    AutoTracker* getAutoTracker() { return _autoTracker; }
    
    struct MemoryWatch
    {
        int callback;
        int addr;
        int len;
        int interval;
        std::string name;
        std::vector<uint8_t> data;
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
    std::list<MemoryWatch> _memoryWatches;
    std::map<std::string, CodeWatch> _codeWatches;
    std::map<std::string, VarWatch> _varWatches;
    AutoTracker *_autoTracker = nullptr;
    
    bool _removeMemoryWatch(const std::string& name);

protected: // Lua interface implementation
    static constexpr const char Lua_Name[] = "ScriptHost";
    static const LuaInterface::MethodMap Lua_Methods;
};


#endif // _CORE_SCRIPTHOST_H
