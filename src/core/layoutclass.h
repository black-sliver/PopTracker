#ifndef _CORE_LAYOUTCLASS_H
#define _CORE_LAYOUTCLASS_H

#include <list>
#include <string>
#include <nlohmann/json.hpp>
#include "direction.h"
#include <climits>
#include <optional>
#include "layouttypes.h"

class LayoutClass final {
public:
    void from_json(const nlohmann::json& j);
    // TODO: more getters
    const std::optional<LayoutTypes::Size>& getItemSize() const { return _itemSize; }
    const std::optional<LayoutTypes::Size>& getSize() const { return _size; }
    const std::optional<LayoutTypes::Size>& getMaxSize() const { return _maxSize; }
    const std::optional<int>& getMaxWidth() const { return _maxWidth; }
    const std::optional<int>& getMaxHeight() const { return _maxHeight; }
    const std::optional<LayoutTypes::Spacing>& getMargin() const { return _margin; }
    const std::optional<LayoutTypes::Orientation>& getOrientation() const { return _orientation; }
    const std::optional<Direction>& getDock() const { return _dock; }
    const std::optional<std::string>& getHAlignment() const { return _hAlignment; }
    const std::optional<std::string>& getVAlignment() const { return _vAlignment; }
    const std::optional<std::string>& getBackground() const { return _background; }
    const std::optional<LayoutTypes::Size>& getItemMargin() const { return _itemMargin; }
    const std::optional<std::string>& getItemHAlignment() const { return _itemHAlign; }
    const std::optional<std::string>& getItemVAlignment() const { return _itemVAlign; }
    const std::optional<LayoutTypes::Size>& getPosition() const { return _position; }
    const std::optional<bool>& getDropShadow() const { return _dropShadow; }

    static LayoutTypes::Size to_size(const nlohmann::json& j, LayoutTypes::Size dflt);
    static LayoutTypes::Size to_pixel(const LayoutTypes::Size& size);
    static LayoutTypes::Orientation to_orientation(const nlohmann::json& j, LayoutTypes::Orientation fallback);
    static LayoutTypes::Spacing to_spacing(const nlohmann::json& j, LayoutTypes::Spacing dflt);
    
protected:
    std::optional<std::string> _background; // TODO: RGBA defaulting to (0,0,0,0) ?
    std::optional<std::string> _hAlignment; // TODO: enum class
    std::optional<std::string> _vAlignment; // TODO: enum class
    std::optional<Direction> _dock;
    std::optional<LayoutTypes::Orientation> _orientation;
    std::optional<std::string> _style;
    std::optional<int> _maxHeight;
    std::optional<int> _maxWidth;
    std::optional<LayoutTypes::Size> _itemSize;
    std::optional<LayoutTypes::Size> _itemMargin;
    std::optional<std::string> _itemHAlign; // TODO: enum class
    std::optional<std::string> _itemVAlign; // TODO: enum class
    std::optional<LayoutTypes::Size> _size;
    std::optional<LayoutTypes::Size> _maxSize;
    std::optional<LayoutTypes::Spacing> _margin;
    std::optional<bool> _compact;
    std::optional<bool> _dropShadow;
    std::optional<LayoutTypes::Size> _position;
};

void from_json(const nlohmann::json& j, LayoutClass& p) {
    p.from_json(j);
}

#endif // _CORE_LayoutClass_H
