#include "layoutnode.h"
#include "util.h"
#include <stdio.h>
#include "jsonutil.h"
using nlohmann::json;


static LayoutNode::OptionalBool to_OptionalBool(const json& j)
{
    if (j.type() == json::value_t::boolean) 
        return j.get<bool>() ? LayoutNode::OptionalBool::True : LayoutNode::OptionalBool::False;
    return LayoutNode::OptionalBool::Undefined;
}
static LayoutNode::Size to_size(const json& j, LayoutNode::Size dflt)
{
    if (j.is_null()) return dflt;
    if (j.is_number()) {
        int x = j.get<int>();
        return {x, x};
    }
    std::string s = to_string(j,"");
    if (s.empty()) return dflt;
    LayoutNode::Size res = dflt;
    char* next = nullptr;
    res.x = (int)strtol(s.c_str(), &next, 0);
    if (next && *next=='.') next = strchr(next, ','); // skip fraction of float
    res.y = (next && *next) ? (int)strtol(next+1, &next, 0) : res.x;
    return res;
}
static int to_pixel(int size) {
    constexpr int icon_sizes[] = {8,16,24,32,48,64,96,128}; // maybe
    return (size<0) ? size :
           (size<countOf(icon_sizes)) ? icon_sizes[size] : size;
}
static LayoutNode::Size to_pixel(const LayoutNode::Size& size) {
    return {to_pixel(size.x), to_pixel(size.y)};
}
static LayoutNode::Orientation to_orientation(const json& j)
{
    std::string s = to_string(j,"");
    if (s == "horizontal") return LayoutNode::Orientation::HORIZONTAL;
    if (s == "vertical")   return LayoutNode::Orientation::VERTICAL;
    return LayoutNode::Orientation::UNDEFINED;
}

static LayoutNode::Spacing to_spacing(const json& j, LayoutNode::Spacing dflt)
{
    if (j.is_null()) return dflt;
    if (j.is_number()) {
        return {j.get<int>()};
    }
    std::string s = to_string(j,"");
    if (s.empty()) return dflt;
    LayoutNode::Spacing res = dflt;
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

LayoutNode LayoutNode::FromJSONString(const std::string& j)
{
    return FromJSON(json::parse(j, nullptr, true, true));
}
LayoutNode LayoutNode::FromJSON(nlohmann::json&& j)
{
    auto tmp = j;
    return FromJSON(tmp);
}
LayoutNode LayoutNode::FromJSON(json& j)
{
    LayoutNode node;
    
    node._type        = to_string(j["type"],""); // TODO: enum
    node._background  = to_string(j["background"],"");
    node._hAlignment  = to_string(j["h_alignment"],""); // TODO: enum
    node._vAlignment  = to_string(j["v_alignment"],""); // TODO: enum
    node._dock        = to_direction(j["dock"]); // TODO: default
    node._orientation = to_orientation(j["orientation"]);
    node._style       = to_string(j["style"],"");
    node._maxHeight   = to_int(j["max_height"],-1);
    node._maxWidth    = to_int(j["max_width"],-1);
    node._itemSize    = to_pixel(to_size(j["item_size"],{-1,-1}));
    node._itemSize.x  = to_int(j["item_width"], node._itemSize.x);
    node._itemSize.y  = to_int(j["item_height"], node._itemSize.y);
    node._itemMargin  = to_size(j["item_margin"],{5,5}); // this is possibly supposed to be left,top,right,bottom
    node._itemHAlign  = to_string(j["item_h_alignment"], "");
    node._itemVAlign  = to_string(j["item_v_alignment"], "");
    node._size.x      = to_int(j["width"], -1);
    node._size.y      = to_int(j["height"], -1);
    node._maxSize.x   = to_int(j["max_width"], -1);
    node._maxSize.y   = to_int(j["max_height"], -1);
    node._margin      = to_spacing(j["margin"], Spacing::UNDEFINED);
        
    node._compact    = to_bool(j["compact"], false);
    node._dropShadow = to_OptionalBool(j["dropshadow"]);
    node._item       = to_string(j["item"],"");
    node._header     = to_string(j["header"],to_string(j["title"],"")); // we use the same variable for groups and tabs
    node._key        = to_string(j["key"],"");
    node._text       = to_string(j["text"],"");
    
    if (j["rows"].type() == json::value_t::array) {
        for (auto& r: j["rows"]) {
            if (r.type() == json::value_t::array) {
                std::list<std::string> row;
                for (auto& v: r) {
                    if (v.type() == json::value_t::string)
                        row.push_back(v);
                    else
                        fprintf(stderr, "WARN: rows item item not string\n");
                }
                node._rows.push_back(row);
            } else {
                fprintf(stderr, "WARN: rows item not array\n");
            }
        }
    }
    else if (j["rows"].type() != json::value_t::null) {
        fprintf(stderr, "WARN: rows not an array\n");
    }
    
    if (j["maps"].type() == json::value_t::array) {
        for (auto& r: j["maps"]) {
            if (r.type() == json::value_t::string)
                node._maps.push_back(r);
            else
                fprintf(stderr, "WARN: maps item not string\n");
        }
    }
    else if (j["maps"].type() != json::value_t::null) {
        fprintf(stderr, "WARN: maps not an array\n");
    }
    
    if (j["content"].type() == json::value_t::object) {
        node._content.push_back(LayoutNode::FromJSON(j["content"]));
    }
    else if (j["content"].type() == json::value_t::array) {
        for (auto& v: j["content"]) {
            if (v.type() == json::value_t::object)
                node._content.push_back(LayoutNode::FromJSON(v));
            else
                fprintf(stderr, "WARN: content item not an object\n");
        }
    }
    else if (j["content"].type() != json::value_t::null) {
        fprintf(stderr, "WARN: content not array or object\n");
    }
    
    // "tabbed" does not directly use content, so we need a special case here
    if (node._type == "tabbed" && j["tabs"].type() == json::value_t::array) {
        for (auto& v: j["tabs"]) {
            if (v.type() == json::value_t::object) {
                v["type"] = "tab";
                node._content.push_back(LayoutNode::FromJSON(v));
            } else {
                fprintf(stderr, "WARN: tabbed item not an object\n");
            }
        }
    } else if (node._type == "tabbed") {
        fprintf(stderr, "WARN: tabs is not an array\n");
    }
    
    return node;
}

