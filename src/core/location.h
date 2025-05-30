#pragma once

#include <list>
#include <string>
#include <luaglue/luainterface.h>
#include <nlohmann/json.hpp>
#include "locationsection.h"


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
            TRAPEZOID,
        };

        static Shape ShapeFromString(const std::string& s) {
            if (s == "rect")
                return Shape::RECT;
            if (s == "diamond")
                return Shape::DIAMOND;
            if (s == "trapezoid")
                return Shape::TRAPEZOID;
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
    static constexpr char Lua_Name[] = "Location";
    static const LuaInterface::MethodMap Lua_Methods;
    int Lua_Index(lua_State *L, const char *key) override;
    bool Lua_NewIndex(lua_State *L, const char *key) override;
};
