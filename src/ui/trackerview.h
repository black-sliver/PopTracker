#ifndef _UI_TRACKERVIEW_H
#define _UI_TRACKERVIEW_H

#include "../uilib/simplecontainer.h"
#include "../uilib/group.h"
#include "../uilib/tabs.h"
#include "../uilib/fontstore.h"
#include "mapwidget.h"
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
    
    Signal<const std::string&> onItemHover;
    
protected:
    Tracker* _tracker;
    std::string _layoutRoot;
    std::list<std::string> _layoutRefs;
    bool _relayoutRequired = false;
    FontStore *_fontStore = nullptr;
    FONT _font = nullptr;
    FONT _smallFont = nullptr;
    Container *_mapTooltip = nullptr;
    Position _mapTooltipPos;
    MapWidget *_mapTooltipOwner = nullptr;
    std::string _mapTooltipName;
    
    int _absX=0;
    int _absY=0;
    std::map<std::string, std::list<Item*>> _items;
    std::map<std::string, std::list<MapWidget*>> _maps;
    std::list<Tabs*> _tabs;
    std::list<std::string> _activeTabs;
    std::list< std::pair<std::string,std::string> > _missedHints;
    
    void updateLayout(const std::string& layout);
    void updateState(const std::string& check);
    void updateLocations();
    
    size_t addLayoutNodes(Container* container, const std::list<LayoutNode>& nodes, size_t depth=0);
    bool addLayoutNode(Container* container, const LayoutNode& node, size_t depth=0);
    
    Item* makeItem(int x, int y, int w, int h, const ::BaseItem& item, int stage1=-1, int stage2=0);
    Item* makeLocationIcon(int x, int y, int w, int h, const std::string& locid, const LocationSection& sec, bool opened, bool compact);
    
    Container* makeMapTooltip(const std::string& location, int x, int y);
    
    int calculateLocationState(const std::string& location);
    
};

} // namespace Ui

#endif // _UI_TRACKERVIEW_H
