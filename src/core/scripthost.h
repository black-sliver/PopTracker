#ifndef _CORE_SCRIPTHOST_H
#define _CORE_SCRIPTHOST_H

#include "../luaglue/luainterface.h"
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
    bool RemoveMemoryWatch(const std::string& name);
    void resetWatches();
    
    bool autoTrack();
    AutoTracker* getAutoTracker() { return _autoTracker; }
    
    struct Watch
    {
        int callback;
        int addr;
        int len;
        int interval;
        std::string name;
        std::vector<uint8_t> data;
    };
    
protected:
    lua_State *_L;
    Pack *_pack;
    Tracker *_tracker;
    std::list<Watch> _watches;
    AutoTracker *_autoTracker = nullptr;
    
protected: // LUA interface implementation
    static constexpr const char Lua_Name[] = "ScriptHost";
    static const LuaInterface::MethodMap Lua_Methods;
};


#endif // _CORE_SCRIPTHOST_H
