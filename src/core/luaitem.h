#ifndef _CORE_LUAITEM_H
#define _CORE_LUAITEM_H

#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "baseitem.h"
#include <luaglue/luainterface.h>
#include <luaglue/luavariant.h>

class LuaItem final : public LuaInterface<LuaItem>, public BaseItem {
    friend class LuaInterface;

public:
    LuaItem() {_type = BaseItem::Type::CUSTOM;}
    
    void Set(const char* key, LuaVariant value);
    LuaVariant Get(const char* key);
    
    virtual bool canProvideCode(const std::string& code) const override;
    int providesCode(const std::string& code) const override;
    virtual bool changeState(Action action) override;
    
    virtual void SetOverlay(const char* text) override {
        if (_overlay == text) return;
        _overlay = text;
        onChange.emit(this);
    }

    virtual void SetOverlayBackground(const char* text) override {
        if (_overlayBackground == text) return;
        _overlayBackground = text;
        onChange.emit(this);
    }

    virtual void SetOverlayAlign(const char* align) override {
        if (_overlayAlign == align) return;
        _overlayAlign = align;
        onChange.emit(this);
    }

    virtual void SetOverlayFontSize(int fontSize) override {
        if (_overlayFontSize == fontSize) return;
        _overlayFontSize = fontSize;
        onChange.emit(this);
    }

    virtual bool setState(int state, int stage=-1) override {
        return false; // TODO: implement this?
    }

    virtual nlohmann::json save() const;
    virtual bool load(nlohmann::json& j);
    
private:
    lua_State *_L = nullptr; // FIXME: fix this
    
    LuaRef _itemState;
    LuaRef _onLeftClickFunc;
    LuaRef _onRightClickFunc;
    LuaRef _onMiddleClickFunc;
    LuaRef _canProvideCodeFunc;
    LuaRef _providesCodeFunc;
    LuaRef _advanceToCodeFunc;
    LuaRef _saveFunc;
    LuaRef _loadFunc;
    LuaRef _propertyChangedFunc;
    LuaRef _potentialIcon;
    std::map<std::string, LuaVariant> _properties; // accessed through Set and Get, only used if _itemState is none

    std::string _fullImg; // including pre-applied mods
    std::list<std::string> _extraImgMods; // set via .IconMods, applied on top of .Icon

    void parseFullImg(); // convert fullImg + _extraImageMods into _img + _imgMods

protected: // Lua interface implementation
    
    static constexpr const char Lua_Name[] = "LuaItem";
    static const LuaInterface::MethodMap Lua_Methods;
    
    virtual int Lua_Index(lua_State *L, const char* key) override;
    virtual bool Lua_NewIndex(lua_State *L, const char *key) override;
};

#endif // _CORE_LUAITEM_H
