#ifndef _CORE_BASEITEM_H
#define _CORE_BASEITEM_H

#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include "signal.h"

class BaseItem { // TODO: move stuff over to JsonItem; TODO: make some stuff pure virtual?
public:
    enum class Type {
        // regular types:
        NONE = 0,
        TOGGLE = 1,
        STATIC, // this could be pseudo-type toggle with allowDisabled=false
        CONSUMABLE,
        PROGRESSIVE,
        // pseudo-types:
        PROGRESSIVE_TOGGLE,
        CUSTOM, // this indicates that most stuff below is useless because it's handled by lua, see TODO above
        UNKNOWN,
    };
    enum class Action { // constants map 1:1 to mouse button
        Primary=1,
        Toggle=2,
        Secondary=3,
        Prev=4,
        Next=5,        
    };
    static Type Str2Type(const std::string& str) {
        if (str == "") return Type::NONE;
        if (str == "toggle") return Type::TOGGLE;
        if (str == "static") return Type::STATIC;
        if (str == "consumable") return Type::CONSUMABLE;
        if (str == "progressive") return Type::PROGRESSIVE;
        if (str == "progressive_toggle") return Type::PROGRESSIVE_TOGGLE;
        return Type::UNKNOWN;
    }
    static std::string Type2Str(Type t) {
        switch (t) {
            case Type::NONE:        return "";
            case Type::TOGGLE:      return "toggle";
            case Type::STATIC:      return "static";
            case Type::CONSUMABLE:  return "consumable";
            case Type::PROGRESSIVE: return "progressive";
            case Type::PROGRESSIVE_TOGGLE: return "progressive_toggle";
            case Type::CUSTOM:      return "custom";
            default:                return "unknown";
        }
    }
    
    Signal<> onChange;
    
protected:
    std::string _id;
    std::string _name;
    Type _type = Type::NONE;
    std::list<std::string> _codes;
    bool _capturable = false;
    bool _loop = false;
    bool _allowDisabled = false;
    std::string _img;
    std::string _disabledImg;
    std::list<std::string> _imgMods;
    std::list<std::string> _disabledImgMods;
    std::string _overlay;
    
    // saving the item's state as part of the item is complete crap,
    // but we need this for compatibility
    int _stage1 = 0; // for toggle this is on/off. "State"
    int _stage2 = 0; // for toggle this is always 0, for staged item this is the stage. "ActiveStage"
    int _count = 0; // for consumables. "quantity" in json
    int _maxCount = 0; // for consumable
    
public:
    const std::string& getName() const { return _name; }
    const std::string& getID() const { return _id; }
    const Type getType() const { return _type; }
    const bool getCapturable() const { return _capturable; }
    const bool getLoop() const { return _loop; } 
    const bool getAllowDisabled() const { return _allowDisabled; }
    
    void setID(const std::string& id) { _id = id; }
    void setID(uint64_t id) { setID(std::to_string(id)); }
    
    virtual void SetOverlay(const char* text) {
        if (_overlay == text) return;
        _overlay = text;
        onChange.emit(this);
    }
    
    //virtual const std::vector<Stage>& getStages() const { return _stages; }
    
    virtual size_t getStageCount() const { return 0; }
    
    virtual const std::string& getImage(size_t stage) const {
        return _img;
    }
    virtual const std::string& getDisabledImage(size_t stage) const {
        return _disabledImg;
    }
    
    virtual const std::list<std::string>& getImageMods(int stage) const {
        return _imgMods;
    }
    virtual const std::list<std::string>& getDisabledImageMods(int stage) const {
        return _disabledImgMods;
    }
    
    
    virtual int providesCode(const std::string code) const { // FIXME: make this pure?
        if (_count && canProvideCode(code)) return _count;
        return (_stage1 && canProvideCode(code));
    }
    
    virtual bool canProvideCode(const std::string& code) const { // FIXME: make this pure?
        if (std::find(_codes.begin(), _codes.end(), code) != _codes.end()) return true;
        return false;
    }
    
    virtual std::string getCodesString() const;
    virtual int getState() const { return _allowDisabled ? _stage1 : 1; }
    virtual int getActiveStage() const { return _stage2; }
    virtual bool changeState(Action action) { return false; }
    
    int getCount() const { return _count; }
    int getMaxCount() const { return _maxCount; }
    const std::string& getOverlay() const { return _overlay; }
};

#endif // _CORE_ITEM_H
