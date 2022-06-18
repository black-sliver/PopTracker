#ifndef _LUAGLUE_LUAINTERFACE_H
#define _LUAGLUE_LUAINTERFACE_H

#include "luatype.h"
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <map> // TODO: write a constexpr_map for MethodMap instead
#include <string_view>
#include <string>

#include "lua_utils.h"


/* Magic to get a c++ object into lua */
/* you need to declare static Lua_Methods and static Lua_Name in your class */
template <class T>
class LuaInterface : public LuaType {
private:
    static int Lua_IndexWrapper(lua_State *L) {
        T *o = luaL_checkthis(L, -2);
        const char *key = luaL_checkstring(L, -1);
#ifdef DEBUG
        printf("LuaInterface<%s>::Lua_Index(\"%s\")\n", T::Lua_Name, key);
#endif
        auto it = T::Lua_Methods.find(key);
        if (it != T::Lua_Methods.end()) {
            lua_pushcfunction(L, it->second);
            return 1;
        }
        return o->Lua_Index(L, key);
    }
    static int Lua_NewIndexWrapper(lua_State *L) {
        T *o = luaL_checkthis(L, -3);
        const char *key = luaL_checkstring(L, -2);
#ifdef DEBUG
        printf("LuaInterface<%s>::Lua_NewIndex(\"%s\", ...)\n", T::Lua_Name, key);
#endif
        auto it = T::Lua_Methods.find(key);
        if (it != T::Lua_Methods.end()) { // disallow write to method names
            std::string msg = std::string("Can't assign to method \"") + key + "\" of \"" + T::Lua_Name + "\"";
            luaL_error(L, msg.c_str());
            return 0;
        }
        if (!o->Lua_NewIndex(L, key)) {
            std::string msg = std::string("Unknown property \"") + key + "\" for \"" + T::Lua_Name + "\"";
            luaL_error(L, msg.c_str());
            return 0;
        }
        return 0;
    }
    static int Lua_GCWrapper(lua_State *L) {
        T *o = luaL_checkthis(L, 1);
        o->Lua_GC(L);
        return 0;
    }
protected:
    LuaInterface(){}
    virtual int Lua_Index(lua_State *L, const char *key){return 0;}
    virtual bool Lua_NewIndex(lua_State *L, const char *key){return false;}
    virtual void Lua_GC(lua_State *L){}
public:
    virtual ~LuaInterface() {}
    static T* luaL_checkthis(lua_State *L, int narg) {
        return * (T**)luaL_checkudata(L, narg, T::Lua_Name);
    }
    static T* luaL_testthis(lua_State *L, int narg) {
        auto p = (T**)luaL_testudata(L, narg, T::Lua_Name);
        if (p) return *p;
        return nullptr;
    }
    // TODO: replace this by a custom constexpr_map
    using MethodMap = std::map<const std::string_view,lua_CFunction>;
    
    static void Lua_Register(lua_State *L) { // create "Class" in Lua
        // create metatable for this class
        luaL_newmetatable(L, T::Lua_Name);
        lua_pushcfunction(L, Lua_IndexWrapper);
        lua_setfield(L,-2, "__index");
        lua_pushcfunction(L, Lua_NewIndexWrapper);
        lua_setfield(L,-2, "__newindex");
        lua_pushcfunction(L, Lua_GCWrapper);
        lua_setfield(L,-2, "__gc");
        lua_pop(L,1);
#ifdef DEBUG
        printf("%s registered\n", T::Lua_Name);
#endif
    }
    
    virtual void Lua_Push(lua_State *L) const { // pushes instance to Lua stack
        // create userdata=pointer for this instance
        const void **ud = (const void**) lua_newuserdata(L, sizeof(void**));
        *ud = this;
        // set metatable
        luaL_setmetatable(L, T::Lua_Name);
#ifdef DEBUG
        printf("%s instance pushed\n", T::Lua_Name);
#endif
    }
};

#endif // _LUAGLUE_LUAINTERFACE_H
