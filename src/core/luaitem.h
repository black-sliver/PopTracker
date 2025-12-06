#pragma once

#include <string>
#include <list>
#include <nlohmann/json.hpp>
#include "baseitem.h"
#include <luaglue/luainterface.h>
#include <luaglue/luavariant.h>


class LuaItem final : public LuaInterface<LuaItem>, public BaseItem {
    friend class LuaInterface;

public:
    LuaItem()
    {
        _type = Type::CUSTOM;
    }

    void Set(const char* key, const LuaVariant& value);
    LuaVariant Get(const char* key);

    bool canProvideCode(const std::string& code) const override;
    int providesCode(const std::string& code) const override;
    bool changeState(Action action) override;

    void SetOverlay(const char* text) override
    {
        if (_overlay == text) return;
        _overlay = text;
        onChange.emit(this);
    }

    void SetOverlayBackground(const char* text) override
    {
        if (_overlayBackground == text) return;
        _overlayBackground = text;
        onChange.emit(this);
    }

    void SetOverlayAlign(const char* align) override
    {
        if (_overlayAlign == align) return;
        _overlayAlign = align;
        onChange.emit(this);
    }

    void SetOverlayFontSize(int fontSize) override
    {
        if (_overlayFontSize == fontSize) return;
        _overlayFontSize = fontSize;
        onChange.emit(this);
    }

    void SetOverlayColor(const char* text) override
    {
        if (_overlayColor == text)
            return;
        _overlayColor = text;
        onChange.emit(this);
    }

    bool setState([[maybe_unused]] int state, [[maybe_unused]] int stage=-1) override
    {
        return false; // TODO: implement this?
    }

    /// Set source filename and source line number for use in debugging and stable ID generation.
    void setSource(const std::string& filename, const int line)
    {
        _sourceFileName = filename;
        _sourceLine = line;
    }

    /// Set source filename and source line number for use in debugging and stable ID generation.
    void setSource(std::string&& filename, const int line)
    {
        _sourceFileName = filename;
        _sourceLine = line;
    }

    /// Returns the source filename the Lua item was created in, if available.
    const std::string& getSourceFilename() const
    {
        return _sourceFileName;
    }

    /// Returns the source line number the Lua item was created in, if available.
    int getSourceLine() const
    {
        return _sourceLine;
    }

    nlohmann::json save() const;
    bool load(nlohmann::json& j);
    
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

    std::string _sourceFileName;
    int _sourceLine = -1;

    void parseFullImg(); // convert fullImg + _extraImageMods into _img + _imgMods

protected: // Lua interface implementation
    
    static constexpr char Lua_Name[] = "LuaItem";
    static const MethodMap Lua_Methods;
    
    int Lua_Index(lua_State *L, const char* key) override;
    bool Lua_NewIndex(lua_State *L, const char *key) override;
};
