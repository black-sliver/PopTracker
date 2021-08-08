#ifndef _CORE_LOCATION_H
#define _CORE_LOCATION_H

#include <list>
#include <string>
#include <nlohmann/json.hpp>
#include "../luaglue/luainterface.h"
#include "../core/signal.h"

    
class LocationSection final : public LuaInterface<LocationSection> {
    friend class LuaInterface;
public:
    static LocationSection FromJSON(nlohmann::json& j, const std::list< std::list<std::string> >& parentRules={}, const std::string& closedImg="", const std::string& openedImg="");
    Signal<> onChange;
protected:
    std::string _name;
    bool _clearAsGroup=false;
    std::string _closedImg;
    std::string _openedImg;
    int _itemCount=0;
    int _itemCleared=0;
    std::list<std::string> _hostedItems;
    std::list< std::list<std::string> > _accessRules;
public:
    // getters
    const std::string& getName() const { return _name; }
    const std::list< std::list<std::string> > getRules() const { return _accessRules; }
    int getItemCount() const { return _itemCount; }
    int getItemCleared() const { return _itemCleared; }
    bool clearItem();
    bool unclearItem();
    const std::string& getClosedImage() const { return _closedImg; }
    const std::string& getOpenedImage() const { return _openedImg; }
    const std::list<std::string>& getHostedItems() const { return _hostedItems; }
    
    virtual nlohmann::json save() const;
    virtual bool load(nlohmann::json& j);
    
protected: // lua interface
    static constexpr const char Lua_Name[] = "LocationSection";
    static const LuaInterface::MethodMap Lua_Methods;
    virtual int Lua_Index(lua_State *L, const char *key) override;
    virtual bool Lua_NewIndex(lua_State *L, const char *key) override;
};

class Location final {
public:
    class MapLocation final {
    public:
        static MapLocation FromJSON(nlohmann::json& j);
    protected:
        std::string _mapName;
        int _x=0; // TODO: point ?
        int _y=0;
    public:
        // getters
        const std::string& getMap() const { return _mapName; }
        int getX() const { return _x; }
        int getY() const { return _y; }
    };
    
    static std::list<Location> FromJSON(nlohmann::json& j, const std::list< std::list<std::string> >& parentRules={}, const std::string& parentName="", const std::string& closedImg="", const std::string& openedImg="");
    
protected:
    std::string _name;
    std::string _parentName;
    std::string _id;
    //std::list< std::list<std::string> > _accessRules;
    std::list<MapLocation> _mapLocations;
    std::list<LocationSection> _sections;
public:
    const std::string& getName() const { return _name; }
    const std::string& getID() const { return _id; }
    const std::list<MapLocation>& getMapLocations() const { return _mapLocations; }
    std::list<LocationSection>& getSections() { return _sections; }
    const std::list<LocationSection>& getSections() const { return _sections; }
    
#ifndef NDEBUG
    void dump(bool compact=false);
#endif
};

#endif // _CORE_LOCATION_H
