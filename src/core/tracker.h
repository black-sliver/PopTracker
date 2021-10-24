#ifndef _CORE_TRACKER_H
#define _CORE_TRACKER_H

#include "../luaglue/luatype.h"
#include "../luaglue/luainterface.h"
#include "pack.h"
#include "luaitem.h"
#include "jsonitem.h"
#include "map.h"
#include "location.h"
#include "layoutnode.h"
#include "signal.h"
#include <string>
#include <list>
#include <cstddef> // nullptr_t
#include <nlohmann/json.hpp>


class Tracker;
    
class Tracker final : public LuaInterface<Tracker> {
    friend class LuaInterface;
    
public:
    
    Tracker(Pack* pack, lua_State *L);
    virtual ~Tracker();
    
    
    struct Object final : public LuaType {
        // NOTE: we could use (something like) std::variant<...> ?
        enum class RT { NIL, JsonItem, LuaItem, Section } type;
        union {
            JsonItem *jsonItem;
            LuaItem *luaItem;
            LocationSection *section;
        };
        Object (std::nullptr_t val) : type(RT::NIL) {}
        Object (JsonItem *val) : type(RT::JsonItem), jsonItem(val) {}
        Object (LuaItem *val) : type(RT::LuaItem), luaItem(val) {}
        Object (LocationSection *val) : type(RT::Section), section(val) {}
        virtual void Lua_Push(lua_State *L) const { // pushes instance to Lua stack
            if (type == RT::JsonItem) jsonItem->Lua_Push(L);
            else if (type == RT::LuaItem) luaItem->Lua_Push(L);
            else if (type == RT::Section) section->Lua_Push(L);
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
    Signal<const std::string&, const std::string&> onUiHint;
    
    const LayoutNode& getLayout(const std::string& name) const;
    bool hasLayout(const std::string& name) const;
    const BaseItem& getItemByCode(const std::string& code) const;
    const BaseItem& getItemById(const std::string& id) const;
    const Map& getMap(const std::string& name) const;
    std::list<std::string> getMapNames() const;
    std::list< std::pair<std::string, Location::MapLocation> > getMapLocations(const std::string& mapname) const;
    Location& getLocation(const std::string& name, bool partialMatch=false);
    
    nlohmann::json saveState() const;
    bool loadState(nlohmann::json& state);
    
    int isReachable(const LocationSection& section);
    
    const Pack* getPack() const;
    
    bool changeItemState(const std::string& id, BaseItem::Action action);
    

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
    std::map<std::string, int> _reachableCache;
    std::map<std::string, int> _providerCountCache;
    bool _bulkUpdate = false;
    
protected: // Lua interface implementation
    static constexpr const char Lua_Name[] = "Tracker";
    static const LuaInterface::MethodMap Lua_Methods;
    
    virtual int Lua_Index(lua_State *L, const char* key) override;
    
};


#endif // _CORE_TRACKER_H
