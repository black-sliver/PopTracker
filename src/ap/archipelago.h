#ifndef _AP_ARCHIPELAGO_H
#define _AP_ARCHIPELAGO_H

// Lua interface to AutoTracker::_ap

#include <luaglue/luainterface.h>
#include <luaglue/lua_json.h>
#include "../core/autotracker.h"
#include "../ap/aptracker.h"

class Archipelago;

class Archipelago : public LuaInterface<Archipelago> {
    friend class LuaInterface;

public:
    Archipelago(lua_State *L, APTracker *ap);
    virtual ~Archipelago();

    bool AddClearHandler(const std::string& name, LuaRef callback);
    bool AddItemHandler(const std::string& name, LuaRef callback);
    bool AddLocationHandler(const std::string& name, LuaRef callback);
    bool AddScoutHandler(const std::string& name, LuaRef callback);
    bool AddBouncedHandler(const std::string& name, LuaRef callback);
    bool AddRetrievedHandler(const std::string& name, LuaRef callback);
    bool AddSetReplyHandler(const std::string& name, LuaRef callback);
    bool SetNotify(const json& keys);
    bool Get(const json& keys);
    bool LocationChecks(const json& locations);
    bool LocationScouts(const json& locations, int sendAsHint);

protected:
    lua_State *_L;
    APTracker *_ap;

protected: // Lua interface implementation
    static constexpr const char Lua_Name[] = "Archipelago";
    static const LuaInterface::MethodMap Lua_Methods;

    virtual int Lua_Index(lua_State *L, const char* key) override;
};


#endif // _AP_ARCHIPELAGO_H

