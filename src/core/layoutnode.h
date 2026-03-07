#ifndef _CORE_LAYOUTNODE_H
#define _CORE_LAYOUTNODE_H

#include <list>
#include <string>
#include <nlohmann/json.hpp>
#include "direction.h"
#include "layoutclass.h"
#include <climits>

class LayoutNode;

class LayoutNode final {
public:

    static LayoutNode FromJSON(nlohmann::json& j, std::unordered_map<std::string, LayoutClass>& classes);
    static LayoutNode FromJSON(nlohmann::json&& j);
    static LayoutNode FromJSONString(const std::string& json, std::unordered_map<std::string, LayoutClass>& classes);
    
protected:
    std::string _type; // TODO: enum class
    std::string _background; // TODO: RGBA defaulting to (0,0,0,0) ?
    std::string _hAlignment; // TODO: enum class
    std::string _vAlignment; // TODO: enum class
    Direction _dock;
    LayoutTypes::Orientation _orientation;
    std::string _style;
    int _maxHeight;
    int _maxWidth;
    LayoutTypes::Size _itemSize;
    LayoutTypes::Size _itemMargin;
    std::string _itemHAlign; // TODO: enum class
    std::string _itemVAlign; // TODO: enum class
    LayoutTypes::Size _size;
    LayoutTypes::Size _maxSize;
    LayoutTypes::Spacing _margin;
    bool _compact;
    LayoutTypes::OptionalBool _dropShadow;
    std::string _item;
    std::string _header;
    std::string _key;
    std::string _text;
    std::list< std::list< std::string > > _rows;
    std::list<std::string> _maps;
    std::list<LayoutNode> _content;
    LayoutTypes::Size _position;

public:
    // TODO: more getters
    const std::string& getType() const { return _type; }
    const std::string& getKey() const { return _key; }
    const std::string& getItem() const { return _item; }
    const std::string& getIcon() const { return _item; /* reuse item for icon since they are exclusive */ }
    const std::string& getHeader() const { return _header; }
    const std::string& getText() const { return _text; }
    const std::list<LayoutNode>& getChildren() const { return _content; }
    const std::list< std::list< std::string > >& getRows() const { return _rows; }
    const LayoutTypes::Size& getItemSize() const { return _itemSize; }
    const LayoutTypes::Size& getSize() const { return _size; }
    const LayoutTypes::Size& getMaxSize() const { return _maxSize; }
    LayoutTypes::Spacing getMargin(const LayoutTypes::Spacing& dflt={5,5,5,5}) const { return _margin==LayoutTypes::Spacing::UNDEFINED ? dflt : _margin; }
    LayoutTypes::Orientation getOrientation() const { return _orientation; }
    Direction getDock() const { return _dock; }
    std::string getHAlignment() const { return _hAlignment; }
    std::string getVAlignment() const { return _vAlignment; }
    const std::string& getBackground() const { return _background; }
    const std::list<std::string>& getMaps() const { return _maps; }
    LayoutTypes::Size getItemMargin() const { return _itemMargin; }
    const std::string& getItemHAlignment() const { return _itemHAlign; }
    const std::string& getItemVAlignment() const { return _itemVAlign; }
    const LayoutTypes::Size& getPosition() const { return _position; }
    LayoutTypes::OptionalBool getDropShadow() const { return _dropShadow; }
    bool getDropShadow(bool dflt) const { return _dropShadow == LayoutTypes::OptionalBool::True ? true :
                                                 _dropShadow == LayoutTypes::OptionalBool::False ? false : dflt; }
};

#endif // _CORE_LAYOUTNODE_H
