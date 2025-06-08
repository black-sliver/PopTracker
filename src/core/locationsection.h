#pragma once

#include <list>
#include <string>
#include <string_view>
#include <luaglue/luainterface.h>
#include <nlohmann/json.hpp>
#include "layoutnode.h" // Size
#include "../core/signal.h"


enum class Highlight : int {
    AVOID = -1,
    NONE = 0,
    NO_PRIORITY = 1,
    UNSPECIFIED = 2,
    PRIORITY = 3,
};


Highlight HighlightFromString(const std::string& name);
std::string_view HighlightToString(Highlight highlight);


class LocationSection final : public LuaInterface<LocationSection> {
    friend class LuaInterface;
public:
    static LocationSection FromJSON(nlohmann::json& j,
            const std::string& parentId,
            const std::list< std::list<std::string> >& parentAccessRules={},
            const std::list< std::list<std::string> >& parentVisibilityRules={},
            const std::string& closedImg="", const std::string& openedImg="",
            const std::string& overlayBackground="");
    Signal<> onChange;

protected:
    std::string _name;
    std::string _parentId;
    bool _clearAsGroup=true;
    std::string _closedImg;
    std::string _openedImg;
    int _itemCount=0;
    int _itemCleared=0;
    std::list<std::string> _hostedItems;
    std::list< std::list<std::string> > _accessRules;
    std::list< std::list<std::string> > _visibilityRules;
    std::string _overlayBackground;
    std::string _ref; // path to actual section if it's just a reference
    Highlight _highlight = Highlight::NONE;
    LayoutNode::Size _itemSize;

public:
    // getters
    const std::string& getName() const { return _name; }
    const std::list< std::list<std::string> > getAccessRules() const { return _accessRules; }
    const std::list< std::list<std::string> > getVisibilityRules() const { return _visibilityRules; }
    int getItemCount() const { return _itemCount; }
    int getItemCleared() const { return _itemCleared; }
    bool clearItem(bool all = false);
    bool unclearItem();
    bool getClearAsGroup() const { return _clearAsGroup; }
    const std::string& getClosedImage() const { return _closedImg; }
    const std::string& getOpenedImage() const { return _openedImg; }
    const std::list<std::string>& getHostedItems() const { return _hostedItems; }
    const std::string& getOverlayBackground() const { return _overlayBackground; }
    const std::string& getParentID() const { return _parentId; }
    std::string getFullID() const { return _parentId + "/" + _name; }
    const std::string& getRef() const { return _ref; }
    Highlight getHighlight() const { return _highlight; }
    const LayoutNode::Size& getItemSize() const { return _itemSize; }

    void setParentID(const std::string& id) { _parentId = id; }

    nlohmann::json save() const;
    bool load(nlohmann::json& j);

    bool operator<(const LocationSection& rhs) const;

protected: // lua interface
    static constexpr char Lua_Name[] = "LocationSection";
    static const LuaInterface::MethodMap Lua_Methods;
    int Lua_Index(lua_State *L, const char *key) override;
    bool Lua_NewIndex(lua_State *L, const char *key) override;
};
