#ifndef _CORE_TRACKER_H
#define _CORE_TRACKER_H

#include <luaglue/luatype.h>
#include <luaglue/luainterface.h>
#include "pack.h"
#include "luaitem.h"
#include "jsonitem.h"
#include "map.h"
#include "location.h"
#include "layoutnode.h"
#include "signal.h"
#include <string>
#include <list>
#include <set>
#include <functional>
#include <cstddef> // nullptr_t
#include <nlohmann/json.hpp>


class Tracker;
    
class Tracker final : public LuaInterface<Tracker> {
    friend class LuaInterface;
    
public:
    static constexpr int DEFAULT_EXEC_LIMIT = 500000;
    
    Tracker(Pack* pack, lua_State *L);
    virtual ~Tracker();

    static int luaErrorHandler(lua_State *L);
    static void luaTimeoutHook(lua_State *L, lua_Debug *);
    
    // TODO: Use a helper to access Lua. This code doesn't belong in tracker
    // Attempt to call a lua func and return an integer value
    static int runLuaFunction(lua_State *L, const std::string name);
    // Attempt to call a lua func. Returns 0 on success
    // arg out is an output that gives the returned value from lua
    static int runLuaFunction(lua_State *L, const std::string name, int &out);
    
    struct Object final : public LuaType {
        // NOTE: we could use (something like) std::variant<...> ?
        enum class RT { NIL, JsonItem, LuaItem, Section, Location } type;
        union {
            JsonItem *jsonItem;
            LuaItem *luaItem;
            LocationSection *section;
            Location *location;
        };
        Object (std::nullptr_t val) : type(RT::NIL) {}
        Object (JsonItem *val) : type(RT::JsonItem), jsonItem(val) {}
        Object (LuaItem *val) : type(RT::LuaItem), luaItem(val) {}
        Object (LocationSection *val) : type(RT::Section), section(val) {}
        Object (Location *val) : type(RT::Location), location(val) {}
        virtual void Lua_Push(lua_State *L) const { // pushes instance to Lua stack
            if (type == RT::JsonItem) jsonItem->Lua_Push(L);
            else if (type == RT::LuaItem) luaItem->Lua_Push(L);
            else if (type == RT::Section) section->Lua_Push(L);
            else if (type == RT::Location) location->Lua_Push(L);
            else lua_pushnil(L);
        }
    };
    
    
    bool AddItems(const std::string& file);
    bool AddLocations(const std::string& file);
    bool AddMaps(const std::string& file);
    bool AddLayouts(const std::string& file);
    int ProviderCountForCode(const std::string& code);
    Object FindObjectForCode(const char* code);
    LuaItem *CreateLuaItem();
    void UiHint(const std::string& name, const std::string& value);
    
    Signal<const LocationSection&> onLocationSectionChanged;
    Signal<const std::string&> onLayoutChanged;
    Signal<const std::string&> onStateChanged;
    Signal<const std::string&> onDisplayChanged; // changed display of an item
    Signal<const std::string&, const std::string&> onUiHint;
    Signal<> onBulkUpdateDone;

    const LayoutNode& getLayout(const std::string& name) const;
    bool hasLayout(const std::string& name) const;
    const BaseItem& getItemByCode(const std::string& code) const;
    BaseItem& getItemById(const std::string& id);
    const Map& getMap(const std::string& name) const;
    std::list<std::string> getMapNames() const;
    std::list< std::pair<std::string, Location::MapLocation> > getMapLocations(const std::string& mapname) const;
    Location& getLocation(const std::string& name, bool partialMatch=false);
    std::pair<Location&, LocationSection&> getLocationAndSection(const std::string& id);
    LocationSection& getLocationSection(const std::string& id);
    const std::vector<std::pair<std::reference_wrapper<const Location>, std::reference_wrapper<const LocationSection>>>&
    getReferencingSections(const LocationSection& sec);

    nlohmann::json saveState() const;
    bool loadState(nlohmann::json& state);

    AccessibilityLevel isReachable(const Location& location, const LocationSection& section);
    bool isVisible(const Location& location, const LocationSection& section);
    AccessibilityLevel isReachable(const Location& location);
    AccessibilityLevel isReachable(const LocationSection& location);
    bool isVisible(const Location& location);
    bool isVisible(const Location::MapLocation& mapLoc);

    bool isBulkUpdate() const;
    bool allowDeferredLogicUpdate() const;

    const Pack* getPack() const;

    bool changeItemState(const std::string& id, BaseItem::Action action);

    static void setExecLimit(int execLimit);
    static int getExecLimit();

protected:
    Pack* _pack;
    lua_State *_L; // TODO: get rid of this by providing an interface to access lua globals
    // NOTE: Items and Locations can not be moved in memory when we share them with Lua
    uint64_t _lastItemID=0;
    std::list<JsonItem> _jsonItems;
    std::list<LuaItem> _luaItems;
    std::list<Location> _locations;
    std::map<std::string, LayoutNode> _layouts;
    std::map<std::string, Map> _maps;
    std::map<std::string, AccessibilityLevel> _accessibilityCache;
    std::map<std::string, bool> _visibilityCache;
    std::map<std::string, int> _providerCountCache;
    std::map<const std::string, Object, std::less<>> _objectCache;
    std::list<std::string> _bulkItemUpdates;
    std::list<std::string> _bulkItemDisplayUpdates;
    bool _bulkUpdate = false;
    bool _allowDeferredLogicUpdate = false; /// opt-in flag to use onBulkUpdate to update state just once at the end
    bool _accessibilityStale = false;
    bool _visibilityStale = false;
    bool _isIndirectConnection = false; /// flag to skip cache for codes that depend on locations
    bool _updatingCache = false; /// true while cache*() is running
    std::set<std::string> _itemChangesDuringCacheUpdate;

    std::map<std::string, std::vector<std::string>> _sectionNameRefs;
    std::map<std::reference_wrapper<const LocationSection>,
             std::vector<std::pair<std::reference_wrapper<const Location>,
                                   std::reference_wrapper<const LocationSection>>>,
             std::less<const LocationSection&>> _sectionRefs;

    static int _execLimit;

    AccessibilityLevel resolveRules(const std::list< std::list<std::string> >& rules, bool visibilityRules);

    void rebuildSectionRefs();
    void cacheAccessibility();
    void cacheVisibility();

protected: // Lua interface implementation
    static constexpr const char Lua_Name[] = "Tracker";
    static const LuaInterface::MethodMap Lua_Methods;
    
    virtual int Lua_Index(lua_State *L, const char* key) override;
    virtual bool Lua_NewIndex(lua_State *L, const char* key) override;
};


#endif // _CORE_TRACKER_H
