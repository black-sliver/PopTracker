#include "layoutnode.h"
#include "util.h"
#include <stdio.h>
#include "jsonutil.h"
using nlohmann::json;


LayoutNode LayoutNode::FromJSONString(const std::string& j, std::unordered_map<std::string, const LayoutClass>& classes)
{
    auto tmp = json::parse(j, nullptr, true, true);
    return FromJSON(tmp, classes);
}

LayoutNode LayoutNode::FromJSON(nlohmann::json&& j)
{
    auto tmp = j;
    std::unordered_map<std::string, const LayoutClass> tmp2;
    return FromJSON(tmp, tmp2);
}

LayoutNode LayoutNode::FromJSON(json& j, std::unordered_map<std::string, const LayoutClass>& classes)
{
    LayoutNode node;
    LayoutClass defClass = {};
    const auto classObjIt = classes.find(to_string(j["class"], ""));
    const LayoutClass& classObj = classObjIt != classes.end() ? classObjIt->second : defClass;

    // Members not overridden by class
    node._type       = to_string(j["type"], ""); // TODO: enum
    node._compact    = to_bool(j["compact"], false);
    node._item       = to_string(j["item"], to_string(j["icon"], "")); // we use the same variable for items and tabs
    node._header     = to_string(j["header"], to_string(j["title"],"")); // we use the same variable for groups and tabs
    node._key        = to_string(j["key"],"");
    node._text       = to_string(j["text"],"");
    node._position.x = to_int(j["canvas_left"], to_int(j["left"], 0));
    node._position.y = to_int(j["canvas_top"], to_int(j["top"], 0));

    // Members overridden by class
    node._background  = to_string(j["background"], classObj.getBackground().value_or(""));
    node._hAlignment  = to_string(j["h_alignment"], classObj.getHAlignment().value_or("")); // TODO: enum
    node._vAlignment  = to_string(j["v_alignment"], classObj.getVAlignment().value_or("")); // TODO: enum
    node._dock        = to_direction(j["dock"], classObj.getDock().value_or(Direction::UNDEFINED)); // TODO: default
    node._orientation = LayoutClass::to_orientation(j["orientation"], classObj.getOrientation().value_or(LayoutTypes::Orientation::UNDEFINED));
    node._style       = to_string(j["style"], classObj.getBackground().value_or(""));
    node._maxHeight   = to_int(j["max_height"], classObj.getMaxHeight().value_or(-1));
    node._maxWidth    = to_int(j["max_width"], classObj.getMaxWidth().value_or(-1));
    node._itemSize    = LayoutClass::to_pixel(LayoutClass::to_size(j["item_size"], classObj.getItemSize().value_or({-1,-1})));
    node._itemSize.x  = to_int(j["item_width"], node._itemSize.x);
    node._itemSize.y  = to_int(j["item_height"], node._itemSize.y);
    node._itemMargin  = LayoutClass::to_size(j["item_margin"], classObj.getItemMargin().value_or({5,5})); // this is possibly supposed to be left,top,right,bottom
    node._itemHAlign  = to_string(j["item_h_alignment"], classObj.getItemHAlignment().value_or(""));
    node._itemVAlign  = to_string(j["item_v_alignment"], classObj.getItemVAlignment().value_or(""));
    node._size.x      = to_int(j["width"], classObj.getMaxWidth().value_or(-1));
    node._size.y      = to_int(j["height"], classObj.getMaxWidth().value_or(-1));
    node._maxSize.x   = to_int(j["max_width"], classObj.getMaxWidth().value_or(-1));
    node._maxSize.y   = to_int(j["max_height"], classObj.getMaxWidth().value_or(-1));
    node._margin      = LayoutClass::to_spacing(j["margin"], classObj.getMargin().value_or(LayoutTypes::Spacing::UNDEFINED));
    node._dropShadow  = LayoutClass::to_OptionalBool(j["dropshadow"], classObj.getDropShadow().value_or(LayoutTypes::OptionalBool::Undefined));

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
        node._content.push_back(LayoutNode::FromJSON(j["content"], classes));
    }
    else if (j["content"].type() == json::value_t::array) {
        for (auto& v: j["content"]) {
            if (v.type() == json::value_t::object)
                node._content.push_back(LayoutNode::FromJSON(v, classes));
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
                node._content.push_back(LayoutNode::FromJSON(v, classes));
            } else {
                fprintf(stderr, "WARN: tabbed item not an object\n");
            }
        }
    } else if (node._type == "tabbed") {
        fprintf(stderr, "WARN: tabs is not an array\n");
    }

    if (node._type != "layout" && !node._key.empty()) {
        fprintf(stderr, "WARN: %s node has \"key\"! Did you mean \"type\": \"layout\"?\n",
                sanitize_print(node._type).c_str());
    }
    
    return node;
}

