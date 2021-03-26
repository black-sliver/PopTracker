#ifndef _CORE_LUAITEM_H
#define _CORE_LUAITEM_H

#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <json/json.hpp>
#include "baseitem.h"
#include "../luaglue/luainterface.h"
#include "../luaglue/luavariant.h"

class LuaItem final : public LuaInterface<LuaItem>, public BaseItem {
    friend class LuaInterface;

public:
    LuaItem() {_type = BaseItem::Type::CUSTOM;}
    
    void Set(const char* key, LuaVariant value);
    LuaVariant Get(const char* key);
    
    virtual bool canProvideCode(const std::string& code) const override;
    virtual int providesCode(const std::string code) const override;
    virtual bool changeState(Action action) override;
    
    virtual void SetOverlay(const char* text) override {
        if (_overlay == text) return;
        _overlay = text;
        onChange.emit(this);
    }
    
    virtual nlohmann::json save() const;
    virtual bool load(nlohmann::json& j);
    
protected:
    lua_State *_L = nullptr; // FIXME: fix this
    
    LuaRef _itemState;
    LuaRef _onLeftClickFunc;
    LuaRef _onRightClickFunc;
    LuaRef _canProvideCodeFunc;
    LuaRef _providesCodeFunc;
    LuaRef _advanceToCodeFunc;
    LuaRef _saveFunc;
    LuaRef _loadFunc;
    LuaRef _propertyChangedFunc;
    LuaRef _potentialIcon;
    std::map<std::string, LuaVariant> _properties; // accessed through Set and Get, only used if _itemState is none
    
protected: // LUA interface implementation
    
    static constexpr const char Lua_Name[] = "LuaItem";
    static const LuaInterface::MethodMap Lua_Methods;
    
    virtual int Lua_Index(lua_State *L, const char* key) override;
    virtual bool Lua_NewIndex(lua_State *L, const char *key) override;
};

#endif // _CORE_LUAITEM_H
