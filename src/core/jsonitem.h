#ifndef _CORE_JSONITEM_H
#define _CORE_JSONITEM_H

#include "baseitem.h"
#include <luaglue/luainterface.h>
#include <algorithm>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <unordered_set>
#ifdef _MSC_VER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif


#define JSONITEM_CI_QUIRK // define to get some packs to work that depend on CI codes


class JsonItem final : public LuaInterface<JsonItem>, public BaseItem {
    friend class LuaInterface;

public:
    static JsonItem FromJSON(nlohmann::json& j);
    static JsonItem FromJSON(nlohmann::json&& j);
    static JsonItem FromJSONString(const std::string& json);
    
    class Stage final {
    protected:
        std::vector<std::string> _codes;
        std::list<std::string> _secondaryCodes;
        std::string _img;
        std::string _disabledImg;
        std::list<std::string> _imgMods;
        std::list<std::string> _disabledImgMods;
        bool _inheritCodes = true;
        std::string _name;

    public:
        static Stage FromJSON(nlohmann::json& j);
        static Stage FromJSON(nlohmann::json&& j);
        static Stage FromJSONString(const std::string& json);
        
        const std::string& getImage() const { return _img; }
        const std::string& getDisabledImage() const { return _disabledImg; }
        const std::list<std::string>& getImageMods() const { return _imgMods; }
        const std::list<std::string>& getDisabledImageMods() const { return _disabledImgMods; }
        const std::vector<std::string>& getCodes() const { return _codes; }
        const std::list<std::string>& getSecondaryCodes() const { return _secondaryCodes; }
        std::string getCodesString() const;
        bool hasCode(const std::string& code) const { // NOTE: this is called canProvideCode in lua
#ifdef JSONITEM_CI_QUIRK
            const auto cmp = [&code](const std::string& s) {
                return code.length() == s.length() && strcasecmp(code.c_str(), s.c_str()) == 0;
            };
            return std::find_if(_codes.begin(), _codes.end(), cmp) != _codes.end();
#else
            return std::find(_codes.begin(), _codes.end(), code) != _codes.end();
#endif
        }
        bool hasSecondaryCode(const std::string& code) const {
            return std::find(_secondaryCodes.begin(), _secondaryCodes.end(), code) != _secondaryCodes.end();
        }
        const bool getInheritCodes() const { return _inheritCodes; }
        const std::string& getName() const { return _name; }
    };

protected:
    std::vector<Stage> _stages;
    bool _imgOverridden = false;
    std::string _imgOverride;
    bool _minCountChanged = false;
    bool _maxCountChanged = false;
    bool _overlayBackgroundChanged = false;
    bool _overlayAlignChanged = false;
    bool _overlayFontSizeChanged = false;
    bool _incrementChanged = false;
    bool _decrementChanged = false;
    bool _imgChanged = false;
    bool _ignoreUserInput = false;

#ifdef JSONITEM_CI_QUIRK
    // CI comparison in list is way too expensive, so we create a lowercase set instead
    std::unordered_set<std::string> _ciAllCodes; // includes codes for stages
#endif

public:
    static void toLowerInPlace(std::string& s)
    {
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    }

    static std::string toLower(std::string s)
    {
        toLowerInPlace(s);
        return s;
    }

    virtual size_t getStageCount() const override { return _stages.size(); }
    
    virtual const std::string& getImage(size_t stage) const override {
        if (_type == Type::TOGGLE) return _img;
        if (_stages.size()>stage) return _stages[stage].getImage();
        return _img;
    }
    virtual const std::string& getDisabledImage(size_t stage) const override {
        if (_type == Type::TOGGLE) return _disabledImg;
        if (_stages.size()>stage) return _stages[stage].getDisabledImage();
        return _disabledImg;
    }
    
    virtual const std::list<std::string>& getImageMods(int stage) const override {
        if (_type == Type::TOGGLE) return _imgMods;
        if ((int)_stages.size()>stage) return _stages[stage].getImageMods();
        return _imgMods;
    }
    virtual const std::list<std::string>& getDisabledImageMods(int stage) const override {
        if (_type == Type::TOGGLE) return _disabledImgMods;
        if ((int)_stages.size()>stage) return _stages[stage].getDisabledImageMods();
        return _disabledImgMods;
    }
    
    virtual const std::string& getCurrentName() const override {
        if ((int)_stages.size() > _stage2 && !_stages[_stage2].getName().empty())
            return _stages[_stage2].getName();
        return _name;
    }

#ifdef JSONITEM_CI_QUIRK
    bool canProvideCodeLower(const std::string& code) const
    {
        return _ciAllCodes.count(code);
    }
#endif

    virtual bool canProvideCode(const std::string& code) const override {
#ifdef JSONITEM_CI_QUIRK
        const auto cmp = [&code](const std::string& s) {
            return code.length() == s.length() && strcasecmp(code.c_str(), s.c_str()) == 0;
        };
        if (std::find_if(_codes.begin(), _codes.end(), cmp) != _codes.end()) return true;
#else
        if (std::find(_codes.begin(), _codes.end(), code) != _codes.end()) return true;
#endif
        for (const auto& stage: _stages)
            if (stage.hasCode(code))
                return true;
        return false;
    }

private:
    int providesCodeImpl(const std::string& code, bool assumeCanProvide) const
    {
        if (_type == Type::COMPOSITE_TOGGLE) {
            // composites do not provide left/right codes since that would duplicate numbers
#ifdef JSONITEM_CI_QUIRK
            const auto cmp = [&code](const std::string& s) {
                return code.length() == s.length() && strcasecmp(code.c_str(), s.c_str()) == 0;
            };
            if (std::find_if(_codes.begin(), _codes.end(), cmp) != _codes.end())
#else
            if (std::find(_codes.begin(), _codes.end(), code) != _codes.end())
#endif
                return ((_stage2&1) ? 1 : 0) + ((_stage2&2) ? 1 : 0);
            return 0;
        }

        if ((int)_stages.size() > _stage2) {
            if (_allowDisabled && !_stage1)
                return 0;
            for (int i=_stage2; i>=0; i--) {
                if (_stages[i].hasCode(code))
                    return 1;
                if (!_stages[i].getInheritCodes())
                    break;
            }
            return false;
        }

        if (_count && (assumeCanProvide || canProvideCode(code)))
            return _count;
        return _stage1 && (assumeCanProvide || canProvideCode(code));
    }

public:
#ifdef JSONITEM_CI_QUIRK
    int providesCodeLower(const std::string& code) const
    {
        if (!canProvideCodeLower(code))
            return 0;
        return providesCodeImpl(code, true);
    }
#endif

    int providesCode(const std::string& code) const override
    {
        // TODO: split at ':' for consumables to be able to check for a specific amount?
        return providesCodeImpl(code, false);
    }

    std::string getCodesString() const override;
    const std::vector<std::string>& getCodes(int stage) const;
    
    virtual bool changeState(BaseItem::Action action) override {
        if (_ignoreUserInput)
            return false;
        if (_changeStateImpl(action)) {
            if (_imgOverridden) {
                _imgOverridden = false;
                _imgOverride.clear();
                onChange.emit(this);
                onDisplayChange.emit(this);
            } else {
                onChange.emit(this);
            }
            return true;
        }
        return false;
    }
    virtual bool setState(int state, int stage=-1) override {
        // NOTE: we have to override because upcasting sender from void* in onChange does not work with multiple inheritance
        if (state<0) state = _stage1;
        if (stage<0) stage = _stage2;
        if (_stage1 != state || _stage2 != stage) {
            _stage1 = state;
            _stage2 = stage;
            _imgOverride.clear();
            _imgOverridden = false;
            onChange.emit(this);
            return true;
        }
        return false;
    }
    
    virtual void SetOverlay(const char* text) override {
        if (_overlay == text) return;
        _overlay = text;
        onChange.emit(this);
    }

    virtual void SetOverlayBackground(const char* text) override {
        if (_overlayBackground == text) return;
        _overlayBackgroundChanged = true;
        _overlayBackground = text;
        onChange.emit(this);
    }

    virtual void SetOverlayAlign(const char* align) override {
        if (_overlayAlign == align) return;
        _overlayAlignChanged = true;
        _overlayAlign = align;
        onChange.emit(this);
    }

    virtual void SetOverlayFontSize(int fontSize) override {
        if (_overlayFontSize == fontSize) return;
        _overlayFontSizeChanged = true;
        _overlayFontSize = fontSize;
        onChange.emit(this);
    }

    bool isImageOverridden() const
    {
        return _imgOverridden;
    }

    const std::string& getImageOverride() const
    {
        return _imgOverride;
    }

    virtual nlohmann::json save() const;
    virtual bool load(nlohmann::json& j);
    
protected:
    
    bool _changeStateImpl(Action action);
    
protected: // Lua interface implementation
    
    static constexpr const char Lua_Name[] = "JsonItem";
    static const LuaInterface::MethodMap Lua_Methods;
    
    virtual int Lua_Index(lua_State *L, const char* key) override;
    virtual bool Lua_NewIndex(lua_State *L, const char *key) override;
};

#endif /* _CORE_JSONITEM_H */

