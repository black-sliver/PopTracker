#ifndef _UI_TRACKERVIEW_H
#define _UI_TRACKERVIEW_H

#include "../uilib/simplecontainer.h"
#include "../uilib/timer.h"
#include "../uilib/group.h"
#include "../uilib/tabs.h"
#include "../uilib/fontstore.h"
#include "mapwidget.h"
#include "maptooltip.h"
#include "item.h"
#include "../core/tracker.h"
#include <list>
#include <map>

namespace Ui {

class TrackerView : public SimpleContainer {
public:
    using FONT = Group::FONT;
    TrackerView(int x, int y, int w, int h, Tracker* tracker, const std::string& layoutRoot, FontStore *fontStore);
    ~TrackerView();

    virtual void render(Renderer renderer, int offX, int offY) override;
    virtual void setSize(Size size) override;
    virtual void addChild(Widget* child) override;
    void relayout();

    std::list< std::pair<std::string,std::string> > getHints() const;
    const std::string& getLayoutRoot() const;
    Tracker* getTracker();

    void setHideClearedLocations(bool hide);
    void setHideUnreachableLocations(bool hide);

    Signal<const std::string&> onItemHover;
    Signal<const std::string&> onItemTooltip;

    static int CalculateLocationState(Tracker* tracker, const std::string& location);
    static int CalculateLocationState(Tracker* tracker, const std::string& location,
            const Location::MapLocation& mapLoc);

protected:
    Tracker* _tracker;
    std::string _layoutRoot;
    std::list<std::string> _layoutRefs;
    bool _relayoutRequired = false;
    FontStore *_fontStore = nullptr;
    FONT _font = nullptr;
    FONT _smallFont = nullptr;
    MapTooltip *_mapTooltip = nullptr;
    Position _mapTooltipPos;
    MapWidget *_mapTooltipOwner = nullptr;
    std::string _mapTooltipName;
    std::map<std::string, int> _mapTooltipScrollOffsets;
    std::string _tooltipItem;
    tick_t _tooltipTimer = 0;
    bool _tooltipTriggered = false;

    int _absX=0;
    int _absY=0;
    std::map<std::string, std::list<Item*>> _items;
    std::map<std::string, std::list<MapWidget*>> _maps;
    std::list<Tabs*> _tabs;
    std::list<std::string> _activeTabs;
    std::list< std::pair<std::string,std::string> > _missedHints;

    bool _hideClearedLocations = false;
    bool _hideUnreachableLocations = false;

    int _defaultItemQuality = -1;
    int _defaultMapQuality = 2; // default smooth

    void updateLayout(const std::string& layout);
    void updateDisplay(const std::string& check);
    void updateState(const std::string& check);
    void updateLocations();
    void updateLocation(const std::string& location);
    void updateItem(Item* w, const BaseItem& item);

    size_t addLayoutNodes(Container* container, const std::list<LayoutNode>& nodes, size_t depth=0);
    bool addLayoutNode(Container* container, const LayoutNode& node, size_t depth=0);

    Item* makeItem(int x, int y, int w, int h, const std::string& code);
    Item* makeItem(int x, int y, int w, int h, const ::BaseItem& item, int stage1=-1, int stage2=0);
    Item* makeLocationIcon(int x, int y, int w, int h, const std::string& locid, const LocationSection& sec, bool opened, bool compact);

    ScrollVBox* makeMapTooltip(const std::string& location, int x, int y);
};

} // namespace Ui

#endif // _UI_TRACKERVIEW_H
