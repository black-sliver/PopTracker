#include "layoutclass.h"
#include "util.h"
#include <stdio.h>
#include "jsonutil.h"
#include "layouttypes.h"
using nlohmann::json;

LayoutTypes::Size LayoutClass::to_size(const json& j, LayoutTypes::Size dflt)
{
    if (j.is_null()) return dflt;
    if (j.is_number()) {
        int x = j.get<int>();
        return {x, x};
    }
    std::string s = to_string(j,"");
    if (s.empty()) return dflt;
    LayoutTypes::Size res = dflt;
    char* next = nullptr;
    res.x = (int)strtol(s.c_str(), &next, 0);
    if (next && *next=='.') next = strchr(next, ','); // skip fraction of float
    res.y = (next && *next) ? (int)strtol(next+1, &next, 0) : res.x;
    return res;
}

static int to_pixel(const int size)
{
    constexpr int icon_sizes[] = {8,16,24,32,48,64,96,128}; // maybe
    return (size < 0) ? size :
           (size < countOf(icon_sizes)) ? icon_sizes[size] : size;
}

LayoutTypes::Size LayoutClass::to_pixel(const LayoutTypes::Size& size)
{
    return {::to_pixel(size.x), ::to_pixel(size.y)};
}

LayoutTypes::Orientation LayoutClass::to_orientation(const json& j, LayoutTypes::Orientation fallback)
{
    std::string s = to_string(j,"");
    if (s == "horizontal") return LayoutTypes::Orientation::HORIZONTAL;
    if (s == "vertical")   return LayoutTypes::Orientation::VERTICAL;
    return fallback;
}

LayoutTypes::Spacing LayoutClass::to_spacing(const nlohmann::json& j, LayoutTypes::Spacing dflt)
{
    if (j.is_null()) return dflt;
    if (j.is_number()) {
        return LayoutTypes::Spacing{j.get<int>()};
    }
    std::string s = to_string(j,"");
    if (s.empty()) return dflt;
    LayoutTypes::Spacing res = dflt;
    char* next = nullptr;
    res.left = (int)strtol(s.c_str(), &next, 0);
    if (next && *next=='.') next = strchr(next, ','); // skip fraction of float
    res.top = (next && *next) ? (int)strtol(next+1, &next, 0) : res.left;
    if (next && *next=='.') next = strchr(next, ','); // skip fraction of float
    res.right = (next && *next) ? (int)strtol(next+1, &next, 0) : res.left;
    if (next && *next=='.') next = strchr(next, ','); // skip fraction of float
    res.bottom = (next && *next) ? (int)strtol(next+1, &next, 0) : res.top;
    return res;
}

LayoutTypes::OptionalBool LayoutClass::to_OptionalBool(const json& j, LayoutTypes::OptionalBool fallback)
{
    if (j.type() == json::value_t::boolean)
        return j.get<bool>() ? LayoutTypes::OptionalBool::True : LayoutTypes::OptionalBool::False;
    return fallback;
}

void LayoutClass::from_json(const json& j)
{
    if (j.contains("background"))
        j.at("background").get_to(_background);
    if (j.contains("h_alignment"))
        j.at("h_alignment").get_to(_hAlignment);

    if (j.contains("v_alignment"))
        j.at("v_alignment").get_to(_vAlignment);

    if (j.contains("style"))
        j.at("style").get_to(_style);
    if (j.contains("max_height"))
        j.at("max_height").get_to(_maxHeight);
    if (j.contains("max_width"))
        j.at("max_width").get_to(_maxWidth);
    if (j.contains("item_width") || j.contains("item_height")) {
        _itemSize = {-1, -1};
        if (j.contains("item_width"))
            j.at("item_width").get_to(_itemSize->x);
        if (j.contains("item_height"))
            j.at("item_height").get_to(_itemSize->y);
    }
    if (j.contains("item_size")) {
        j.at("item_size").get_to(_itemSize);
    }
    if (j.contains("item_h_alignment"))
        j.at("item_h_alignment").get_to(_itemHAlign);
    if (j.contains("item_v_alignment"))
        j.at("item_v_alignment").get_to(_itemVAlign);
    if (j.contains("dropshadow"))
        j.at("dropshadow").get_to(_dropShadow);
    if (j.contains("width") || j.contains("height")) {
        _size = {-1, -1};
        if (j.contains("width"))
            j.at("width").get_to(_size->x);
        if (j.contains("height"))
            j.at("height").get_to(_size->y);
    }
    if (j.contains("max_width") || j.contains("max_height")) {
        _maxSize = {-1, -1};
        if (j.contains("max_width"))
            j.at("max_width").get_to(_maxSize->x);
        if (j.contains("max_height"))
            j.at("max_height").get_to(_maxSize->y);
    }
    if (j.contains("dropShadow"))
        _dropShadow = LayoutClass::to_OptionalBool(j["dropshadow"], LayoutTypes::OptionalBool::Undefined);
    if (j.contains("dock"))
        _dock = to_direction(j["dock"]); // TODO: default
    if (j.contains("orientation"))
        _orientation = to_orientation(j["orientation"], LayoutTypes::Orientation::UNDEFINED);
    if (j.contains("item_margin"))
        // this is possibly supposed to be left,top,right,bottom
        _itemMargin = to_size(j["item_margin"],LayoutTypes::Size({5,5}));
    if (j.contains("margin"))
        _margin = to_spacing(j["margin"], LayoutTypes::Spacing::UNDEFINED);
}
