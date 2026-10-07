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
            TRIANGLE,
            CONCAVE_KITE,
        };

        enum class Orientation {
            UNSPECIFIED,
            NORTH,
            SOUTH,
            EAST,
            WEST,
        };

        static Shape ShapeFromString(const std::string& s) {
            if (s == "rect")
                return Shape::RECT;
            if (s == "diamond")
                return Shape::DIAMOND;
            if (s == "trapezoid")
                return Shape::TRAPEZOID;
            if (s == "triangle")
                return Shape::TRIANGLE;
            if (s == "concave_kite")
                return Shape::CONCAVE_KITE;
            return Shape::UNSPECIFIED;
        }

        static constexpr float ShapeBorderStrengthCoefficient(Shape s) {
            // if C is the below coefficient, then thickness = C * strength.
            switch (s) {
            case Shape::UNSPECIFIED:
            case Shape::RECT:
            case Shape::TRAPEZOID:
                return 1.0f;
            case Shape::DIAMOND:
                return 1.41421356237310f; // equal to sqrt(2)
            case Shape::TRIANGLE:
                return 2.23606797749979f; // equal to sqrt(5)
            case Shape::CONCAVE_KITE:
                return 2.92080962648189f; // equal to (sqrt(5)+sqrt(13))/2
            }
            return 0.0f;
        }

        static Orientation OrientationFromString(const std::string& s) {
            if (s == "north")
                return Orientation::NORTH;
            else if (s == "south")
                return Orientation::SOUTH;
            else if (s == "east")
                return Orientation::EAST;
            else if (s == "west")
                return Orientation::WEST;
            return Orientation::UNSPECIFIED;
        }

    protected:
        std::string _mapName;
        int _x = 0; // TODO: point ?
        int _y = 0;
        int _size = -1;
        int _borderThickness = -1;
        int _borderStrength = -1;
        std::list<std::list<std::string> > _visibilityRules;
        std::list<std::list<std::string> > _invisibilityRules;
        Shape _shape = Shape::UNSPECIFIED;
        Orientation _orientation = Orientation::NORTH;

    public:
        // getters
        const std::string& getMap() const { return _mapName; }
        int getX() const { return _x; }
        int getY() const { return _y; }
        int getSize(int parent) const { return _size < 1 ? parent : _size; }
        float getBorderThickness(Shape shape, int parentThickness, int parentStrength) const {
            if (_borderStrength >= 0) {
                return _borderStrength * ShapeBorderStrengthCoefficient(shape);
            } else if (_borderThickness >= 0) {
                return _borderThickness;
            } else if (parentStrength >= 0) {
                return parentStrength * ShapeBorderStrengthCoefficient(shape);
            } else {
                return parentThickness;
            }
        }
        Shape getShape(Shape parent) const
        {
            return _shape == Shape::UNSPECIFIED ? parent : _shape;
        }
        Orientation getOrientation(Orientation parent) const
        {
            return _orientation == Orientation::UNSPECIFIED ? parent : _orientation;
        }

        const std::list<std::list<std::string>>& getVisibilityRules() const { return _visibilityRules; }
        const std::list<std::list<std::string>>& getInvisibilityRules() const { return _invisibilityRules; }
    };

    static std::list<Location> FromJSON(
        nlohmann::json& j,
        const std::deque<Location>& parentLookup,
        bool glitchedScoutableAsGlitched = false,
        const std::list< std::list<std::string> >& parentAccessRules={},
        const std::list< std::list<std::string> >& parentVisibilityRules={},
        const std::string& closedImg="",
        const std::string& openedImg="",
        const std::string& overlayBackground="",
        const std::string& parentName="");

protected:
    std::string _name;
    std::string _parentName;
    std::string _id;
    std::list<MapLocation> _mapLocations;
    std::list<LocationSection> _sections;
    std::list< std::list<std::string> > _accessRules; // this is only used if referenced through @-Rules
    std::list< std::list<std::string> > _visibilityRules;
    bool _glitchedScoutableAsGlitched = false;

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
    bool getGlitchedScoutableAsGlitched() const { return _glitchedScoutableAsGlitched; }
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
