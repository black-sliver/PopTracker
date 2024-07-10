#ifndef _CORE_LOCATION_H
#define _CORE_LOCATION_H

#include <list>
#include <string>
#include <nlohmann/json.hpp>
#include <luaglue/luainterface.h>
#include "../core/signal.h"


enum class AccessibilityLevel : int {
    NONE = 0,
    PARTIAL = 1,
    INSPECT = 3,
    SEQUENCE_BREAK = 5,
    NORMAL = 6,
    CLEARED = 7,
};


class LocationSection final : public LuaInterface<LocationSection> {
    friend class LuaInterface;
public:
    static LocationSection FromJSON(nlohmann::json& j,
            const std::string parentId,
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

    void setParentID(const std::string& id) { _parentId = id; }

    virtual nlohmann::json save() const;
    virtual bool load(nlohmann::json& j);

    bool operator<(const LocationSection& rhs) const;

protected: // lua interface
    static constexpr const char Lua_Name[] = "LocationSection";
    static const LuaInterface::MethodMap Lua_Methods;
    virtual int Lua_Index(lua_State *L, const char *key) override;
    virtual bool Lua_NewIndex(lua_State *L, const char *key) override;
};

class Location final : public LuaInterface<Location> {
    friend class LuaInterface;
public:
    class MapLocation final {
    public:
        static MapLocation FromJSON(nlohmann::json& j);

        enum class Shape {
            UNSPECIFIED,
            RECT,
            DIAMOND,
        };

        static Shape ShapeFromString(const std::string& s) {
            if (s == "rect")
                return Shape::RECT;
            if (s == "diamond")
                return Shape::DIAMOND;
            return Shape::UNSPECIFIED;
        }

    protected:
        std::string _mapName;
        int _x = 0; // TODO: point ?
        int _y = 0;
        int _size = -1;
        int _borderThickness = -1;
        std::list<std::list<std::string> > _visibilityRules;
        std::list<std::list<std::string> > _invisibilityRules;
        Shape _shape = Shape::UNSPECIFIED;

    public:
        // getters
        const std::string& getMap() const { return _mapName; }
        int getX() const { return _x; }
        int getY() const { return _y; }
        int getSize(int parent) const { return _size < 1 ? parent : _size; }
        int getBorderThickness(int parent) const { return _borderThickness < 0 ? parent : _borderThickness; }
        Shape getShape(Shape parent) const
        {
            return _shape == Shape::UNSPECIFIED ? parent : _shape;
        }

        const std::list<std::list<std::string>>& getVisibilityRules() const { return _visibilityRules; }
        const std::list<std::list<std::string>>& getInvisibilityRules() const { return _invisibilityRules; }
    };

    static std::list<Location> FromJSON(nlohmann::json& j,
        const std::list<Location>& parentLookup,
        const std::list< std::list<std::string> >& parentAccessRules={},
        const std::list< std::list<std::string> >& parentVisibilityRules={},
        const std::string& parentName="", const std::string& closedImg="",
        const std::string& openedImg="", const std::string& overlayBackground="");

protected:
    std::string _name;
    std::string _parentName;
    std::string _id;
    std::list<MapLocation> _mapLocations;
    std::list<LocationSection> _sections;
    std::list< std::list<std::string> > _accessRules; // this is only used if referenced through @-Rules
    std::list< std::list<std::string> > _visibilityRules;
public:
    const std::string& getName() const { return _name; }
    const std::string& getID() const { return _id; }
    void setID(const std::string& id) { _id = id; }
    const std::list<MapLocation>& getMapLocations() const { return _mapLocations; }
    std::list<LocationSection>& getSections() { return _sections; }
    const std::list<LocationSection>& getSections() const { return _sections; }
    std::list< std::list<std::string> >& getAccessRules() { return _accessRules; }
    const std::list< std::list<std::string> >& getAccessRules() const { return _accessRules; }
    std::list< std::list<std::string> >& getVisibilityRules() { return _visibilityRules; }
    const std::list< std::list<std::string> >& getVisibilityRules() const { return _visibilityRules; }
    void merge(const Location& other);

#ifndef NDEBUG
    void dump(bool compact=false);
#endif

protected: // lua interface
    static constexpr const char Lua_Name[] = "Location";
    static const LuaInterface::MethodMap Lua_Methods;
    virtual int Lua_Index(lua_State *L, const char *key) override;
    virtual bool Lua_NewIndex(lua_State *L, const char *key) override;
};

#endif // _CORE_LOCATION_H
