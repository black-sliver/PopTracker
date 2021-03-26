#ifndef _CORE_LAYOUTNODE_H
#define _CORE_LAYOUTNODE_H

#include <list>
#include <string>
#include <json/json.hpp>
#include "direction.h"

class LayoutNode;

class LayoutNode final {
public:
    enum class OptionalBool {
        Undefined=-1,
        False=0,
        True=1,
    };
    enum class Orientation {
        UNDEFINED=0,
        HORIZONTAL=1,
        VERTICAL=2,
    };
    
    struct Size final {
        int x;
        int y;
        constexpr Size(int w, int h) : x(w), y(h) {}
        constexpr Size() : x(0), y(0) {}
    };
    
    static LayoutNode FromJSON(nlohmann::json& j);
    static LayoutNode FromJSON(nlohmann::json&& j);
    static LayoutNode FromJSONString(const std::string& json);
    
protected:
    std::string _type; // TODO: enum class
    std::string _background; // TODO: RGBA defaulting to (0,0,0,0) ?
    std::string _hAlignment; // TODO: enum class
    std::string _vAlignment; // TODO: enum class
    Direction _dock;
    Orientation _orientation;
    std::string _style;
    int _maxHeight;
    int _maxWidth;
    Size _itemSize;
    Size _itemMargin;
    Size _size;
    Size _maxSize;
    bool _compact;
    OptionalBool _dropShadow;
    std::string _item;
    std::string _header;
    std::string _key;
    std::list< std::list< std::string > > _rows;
    std::list<std::string> _maps;
    std::list<LayoutNode> _content;
public:
    // TODO: more getters
    const std::string& getType() const { return _type; }
    const std::string& getKey() const { return _key; }
    const std::string& getItem() const { return _item; }
    const std::string& getHeader() const { return _header; }
    const std::list<LayoutNode>& getChildren() const { return _content; }
    const std::list< std::list< std::string > >& getRows() const { return _rows; }
    const Size& getItemSize() const { return _itemSize; }
    const Size& getSize() const { return _size; }
    const Size& getMaxSize() const { return _maxSize; }
    Orientation getOrientation() const { return _orientation; }
    Direction getDock() const { return _dock; }
    std::string getHAlignment() const { return _hAlignment; }
    std::string getVAlignment() const { return _vAlignment; }
    const std::string& getBackground() const { return _background; }
    const std::list<std::string>& getMaps() const { return _maps; }
    Size getItemMargin() const { return _itemMargin; }
};

#endif // _CORE_LAYOUTNODE_H
