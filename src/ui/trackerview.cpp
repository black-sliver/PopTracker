#include "trackerview.h"
#include "../uilib/image.h"
#include "../uilib/simplecontainer.h"
#include "../uilib/hbox.h"
#include "../uilib/vbox.h"
#include "../uilib/dock.h"
#include "../uilib/tabs.h"
#include "../core/fileutil.h"
#include "../core/assets.h"
#include "item.h"
#include "mapwidget.h"
#include "defaults.h" // DEFAULT_FONT_*

namespace Ui {


constexpr int TOOL_OFF=2;
constexpr int TOOL_MAX_DISPLACEMENT=5; // can be off by this amount
constexpr Size TOOL_MIN_SIZE={ 32,32 };

std::list<ImageFilter> imageModsToFilters(Tracker* tracker, const std::list<std::string>& mods)
{
    std::list<ImageFilter> filters;
    for (auto& mod: mods) {
        std::string name, arg;
        size_t p = mod.find('|');
        if (p == mod.npos) name = mod;
        else {
            name = mod.substr(0,p);
            arg  = mod.substr(p+1);
        }
        if (name == "overlay") {
            // read actual image data into arg instead of filename
            std::string tmp = std::move(arg);
            tracker->getPack()->ReadFile(tmp, arg);
        }
        if (name == "@disable" || name == "@disabled")
        {
            name = "grey";
        }
        filters.push_back({name,arg});
    }
    return filters;
}

static Label::HAlign str2itemHalign(const std::string& s, Label::HAlign dflt=Label::HAlign::LEFT)
{
    if (s.empty()) return dflt;
    if (s == "center") return Label::HAlign::CENTER;
    if (s == "right") return Label::HAlign::RIGHT;
    if (s == "left") return Label::HAlign::LEFT;
    return dflt;
}

static Label::VAlign str2itemValign(const std::string& s, Label::VAlign dflt=Label::VAlign::TOP)
{
    if (s.empty()) return dflt;
    if (s == "bottom") return Label::VAlign::BOTTOM;
    if (s == "center" || s == "middle") return Label::VAlign::MIDDLE;
    if (s == "top") return Label::VAlign::TOP;
    return dflt;
}

Item* TrackerView::makeItem(int x, int y, int width, int height, const ::BaseItem& origItem, int stage1, int stage2)
{
    std::string f, s;

    // NOTE: for toggle_badged we use stage 1 (enable/disable) for the badge
    const ::BaseItem *item = &origItem;
    if (!origItem.getBaseItem().empty()) {
        auto o = _tracker->FindObjectForCode(origItem.getBaseItem().c_str());
        if (o.type == Tracker::Object::RT::JsonItem)
            item = o.jsonItem;
        else if (o.type == Tracker::Object::RT::LuaItem)
            item = o.luaItem;
    }

    Item *w = new Item(x,y,width,height,_fontStore->getFont(DEFAULT_FONT_NAME,
            FontStore::sizeFromData(DEFAULT_FONT_SIZE, item->getOverlayFontSize())));
    size_t stages = item->getStageCount();
    bool disabled = item->getAllowDisabled();
    bool stagedWithDisabled = item->getStageCount() && disabled;

    for (size_t n=0; n==0||n<stages; n++) {
        f = item->getImage(n);
        _tracker->getPack()->ReadFile(f, s);
        const auto& mods = item->getImageMods(n);
        auto filters = imageModsToFilters(_tracker, mods);
        if (origItem.getType() == ::BaseItem::Type::TOGGLE_BADGED) {
            // stupid work-around: if base item is staged and has allow_disabled
            // we need to insert an additional stage for disabled state
            size_t m = stagedWithDisabled ? (n+1) : n;
            w->addStage(0,m, s.c_str(), s.length(), filters);
            std::list<std::string> badgeMods = item->getImageMods(n);
            badgeMods.push_back("overlay|" + origItem.getImage(0));
            auto badgeFilters = imageModsToFilters(_tracker, badgeMods);
            w->addStage(1,m, s.c_str(), s.length(), badgeFilters);
            if (n == 0 && m != 0) {
                f = item->getDisabledImage(0);
                _tracker->getPack()->ReadFile(f, s);
                const auto& mods = item->getDisabledImageMods(0);
                filters = imageModsToFilters(_tracker, mods);
                w->addStage(0,0, s.c_str(), s.length(), filters);
                badgeMods = mods;
                badgeMods.push_back("overlay|" + origItem.getImage(0));
                badgeFilters = imageModsToFilters(_tracker, badgeMods);
                w->addStage(1,0, s.c_str(), s.length(), badgeFilters);
            }
        }
        else if (disabled) {
            w->addStage(1,n, s.c_str(), s.length(), filters);
            auto disF = item->getDisabledImage(n);
            if (f != disF) {
                f = disF;
                _tracker->getPack()->ReadFile(f, s);
            }
            auto disMods = item->getDisabledImageMods(n);
            filters = imageModsToFilters(_tracker, disMods);
            //if (!disMods.empty()) filters = imageModsToFilters(_tracker, disMods);
            //if (disMods.empty() || disMods.front() != "none") filters.push_back({"grey"});
            w->addStage(0,n, s.c_str(), s.length(), filters);
        }
        else {
            w->addStage(1,n, s.c_str(), s.length(), filters);
        }
    }
    if (item->getCount()) {
        if (item->getCount() == item->getMaxCount()) {
            w->setOverlayColor({31,255,31});
        } else {
            w->setOverlayColor({220,220,220});
        }
        w->setOverlay(std::to_string(item->getCount()));
    } else {
        w->setOverlay(item->getOverlay());
        w->setOverlayColor({220,220,220});
    }
    w->setOverlayBackgroundColor(item->getOverlayBackground());
    
    std::string id = origItem.getID();
    w->onClick += {this, [this,id] (void *s, int x, int y, int btn) {
        printf("Item %s clicked w/ btn %d!\n", id.c_str(), btn);
        if (btn == BUTTON_LEFT) {
            _tracker->changeItemState(id, ::BaseItem::Action::Primary);
        } else if (btn == BUTTON_RIGHT) {
            _tracker->changeItemState(id, ::BaseItem::Action::Secondary);
        } else if (btn == BUTTON_BACK) {
            _tracker->changeItemState(id, ::BaseItem::Action::Prev);
        } else if (btn == BUTTON_FORWARD) {
            _tracker->changeItemState(id, ::BaseItem::Action::Next);
        } else if (btn == BUTTON_MIDDLE) {
            _tracker->changeItemState(id, ::BaseItem::Action::Toggle);
        }
    }};
    if (!id.empty() && !origItem.getName().empty()) { // skip hover for non-item items
        w->onMouseEnter += {this, [this,id] (void *s, int x, int y, unsigned btns) {
            onItemHover.emit(this, id);
        }};
        w->onMouseLeave += {this, [this] (void *s) {
            onItemHover.emit(this, "");
        }};
    }
    w->onDestroy += {this, [this,id] (void *s) {
        _items[id].remove((Item*)s);
    }};
    _items[id].push_back(w);
    
    if (stage1>=0 && stage2>=0)
        w->setStage(stage1,stage2);
    else if (stagedWithDisabled && origItem.getType() == ::BaseItem::Type::TOGGLE_BADGED)
        w->setStage(origItem.getState(), item->getState() ? (item->getActiveStage()+1) : 0);
    else
        w->setStage(origItem.getState(), item->getActiveStage());
    return w;
}

Item* TrackerView::makeLocationIcon(int x, int y, int width, int height, const std::string& locid, const LocationSection& sec, bool opened, bool compact)
{
    std::string sClosed, sOpened;
    {
        std::string f = sec.getClosedImage();
        _tracker->getPack()->ReadFile(f, sClosed);
        if (sClosed.empty()) {
            readFile(asset("closed.png"), sClosed); // fallback/default icon
        }
    }
    {
        std::string f = sec.getOpenedImage();
        _tracker->getPack()->ReadFile(f, sOpened);
        if (sOpened.empty()) {
            readFile(asset("open.png"), sOpened); // fallback/default icon
        }
    }
    Item *w = new Item(x,y,width,height,_font);
    w->addStage(0,0, sClosed.c_str(), sClosed.length()); // TODO: +img_mods
    w->addStage(1,0, sOpened.c_str(), sOpened.length()); // TODO: +img_mods
    w->setStage(opened?1:0,0);
    w->setMinSize(w->getSize()); // FIXME: this is a dirty work-around
    w->setOverlayBackgroundColor(sec.getOverlayBackground());

    if (compact) {
        int itemcount = sec.getItemCount();
        int looted = sec.getItemCleared();
        if (itemcount!=1 && itemcount>looted) { // TODO: show two numbers: looted + non-looted?
            w->setOverlay(std::to_string(itemcount - looted));
        } else {
            w->setOverlay("");
        }
    }
    
    auto name = sec.getName(); // TODO: id instead of name
    w->onClick += {this, [this,w,locid,name,compact](void*,int x, int y, int btn) {
        if (!compact && btn == BUTTON_RIGHT && w->getStage1()<1) return;
        if (!compact && btn != BUTTON_RIGHT && w->getStage1()>0) return;
        auto& loc = _tracker->getLocation(locid);
        for (auto& sec: loc.getSections()) {
            if (sec.getName() != name) continue;
            // check hosted items with chests if "clear_as_group" is set
            if (sec.getClearAsGroup()) {
                for (const auto& item : sec.getHostedItems()) {
                    if (_tracker->changeItemState(_tracker->getItemByCode(item).getID(),
                            btn == BUTTON_RIGHT ? BaseItem::Action::Secondary : BaseItem::Action::Primary));
                }
            }
            // NOTE: clear/unclear invalidates the iterator because it fires a change signal
            if (btn == BUTTON_RIGHT)
                sec.unclearItem();
            else
                sec.clearItem();
            break;
        }
    }};
    return w;
}


TrackerView::TrackerView(int x, int y, int w, int h, Tracker* tracker, const std::string& layoutRoot, FontStore *fontStore)
    : SimpleContainer(x,y,w,h), _tracker(tracker), _layoutRoot(layoutRoot), _fontStore(fontStore)
{
    _font = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE);
    _smallFont = _fontStore->getFont(DEFAULT_FONT_NAME, DEFAULT_FONT_SIZE - 2);
    if (_font && !_smallFont) _smallFont = _font;
    _tracker->onLayoutChanged += {this, [this](void *s, const std::string& layout) {
        updateLayout(layout);
    }};
    _tracker->onStateChanged += {this, [this](void *s, const std::string& check) {
        updateState(check);
    }};
    _tracker->onLocationSectionChanged += {this, [this](void *s, const LocationSection& sec) {
        updateLocations();
    }};
    updateLayout(layoutRoot);
    updateState("");
}

TrackerView::~TrackerView()
{
    _tracker->onLayoutChanged -= this;
    _tracker->onStateChanged -= this;
    _tracker->onLocationSectionChanged -= this;
    _tracker->onUiHint -= this;
    _tracker = nullptr;
    
    for (auto pair: _items) {
        for (auto w: pair.second) {
            w->onDestroy -= this;
        }
    }
}

void TrackerView::relayout()
{
    const LayoutNode node = _tracker->getLayout(_layoutRoot);
    _relayoutRequired = false;
    _tracker->onUiHint -= this; // stop recording hints
    if (node.getType() != "") addLayoutNode(this, node);
    for (const auto& pair : _missedHints) { // replay missed layout hints
        _tracker->onUiHint.emit(_tracker, pair.first, pair.second);
    }
    _missedHints.clear();
    updateLocations();
}

void TrackerView::render(Renderer renderer, int offX, int offY)
{
    if (_relayoutRequired) {
        auto oldSize = _size;
        setSize({300,200}); // FIXME: we should really fix relayout() at some point
        relayout();
        setSize(oldSize);
    }
    // store global coordinates for overlay calculations
    _absX = offX+_pos.left;
    _absY = offY+_pos.top;
    // render children
    Container::render(renderer, offX, offY);
}

std::list< std::pair<std::string,std::string> > TrackerView::getHints() const
{
    // returns list of hints to restore current Ui/Layout stage
    std::list< std::pair<std::string,std::string> > hints;
    for (auto& tab: _tabs) {
        auto name = tab->getActiveTabName();
        if (!name.empty()) hints.push_back({"ActivateTab", name});
    }
    for (auto& hint: _missedHints) {
        hints.push_back(hint);
    }
    return hints;
}

const std::string& TrackerView::getLayoutRoot() const
{
    return _layoutRoot;
}

Tracker* TrackerView::getTracker()
{
    return _tracker;
}

void TrackerView::updateLayout(const std::string& layout)
{    
    if (layout != "" && layout != _layoutRoot && std::find(_layoutRefs.begin(),_layoutRefs.end(),layout) == _layoutRefs.end()) return;
    
    // store active tabs
    _activeTabs.clear();
    for (const auto tab: _tabs) {
        auto name = tab->getActiveTabName();
        if (!name.empty()) _activeTabs.push_back(name);
    }
    
    // clear ui
    _tracker->onUiHint -= this;
    _mapTooltip = nullptr;
    _mapTooltipOwner = nullptr;
    _items.clear();
    _maps.clear();
    _tabs.clear();
    _layoutRefs.clear();
    clearChildren();
    _relayoutRequired = true;
    
    // record "missed" hints during update to replay them later
    _tracker->onUiHint += { this, [this] (void* s, const std::string& name, const std::string& value) {
        _missedHints.push_back({name,value});
    }};
}

void TrackerView::updateLocations()
{
    for (auto& mappair: _maps) {
        //const auto& map = _tracker->getMap(mappair.first);
        for (auto& w: mappair.second) {
            for (const auto& pair : _tracker->getMapLocations(mappair.first)) {
                int state = calculateLocationState(pair.first);
                if (_maps.size()<1) {
                    printf("TrackerView: UI changed during updateLocations()\n");
                    fprintf(stderr, "cybuuuuuu!!\n");
                    return;
                }
                w->setLocationState(pair.first, state);
            }
        }
    }
    // dirty work-around: just run the hove signal to recreate tooltip
    // FIXME: we probably want to have a custom widget for that
    
    if (_mapTooltip && _mapTooltipOwner) {
        _mapTooltipOwner->onLocationHover.emit(_mapTooltipOwner, _mapTooltipName, _mapTooltipPos.left, _mapTooltipPos.top);
    }
}
void TrackerView::updateState(const std::string& itemid)
{
    const auto& item = _tracker->getItemById(itemid);
    printf("update state of %s: \"%s\"\n", itemid.c_str(), item.getName().c_str());
    for (auto w: _items[itemid]) {
        if (item.getType() == ::BaseItem::Type::CUSTOM) {
            int st = item.getActiveStage();
            auto filters = imageModsToFilters(_tracker, item.getImageMods(st));
            auto f = item.getImage(st);
            std::string s;
            _tracker->getPack()->ReadFile(f, s);
            w->addStage(w->getStage1(), w->getStage2(), s.c_str(), s.length(), filters);
            printf("Image updated!\n");
        } else if (item.getType() == ::BaseItem::Type::TOGGLE_BADGED) {
            // stage is controller by base item, state by badge
            // stupid hack: if the base item is staged and it has
            // allow_disabled, we need to add a "disabled" stage
            int stage = item.getActiveStage();
            auto o = _tracker->FindObjectForCode(item.getBaseItem().c_str());
            if (o.type == Tracker::Object::RT::JsonItem) {
                stage = o.jsonItem->getActiveStage();
                if (o.jsonItem->getStageCount() && o.jsonItem->getAllowDisabled())
                    stage = o.jsonItem->getState() ? stage+1 : 0;
            } else if (o.type == Tracker::Object::RT::LuaItem) {
                stage = o.luaItem->getActiveStage();
                if (o.luaItem->getStageCount()>1 && o.luaItem->getAllowDisabled())
                    stage = o.luaItem->getState() ? stage+1 : 0;
            }
            w->setStage(item.getState(), stage);
        } else {
            w->setStage(item.getState(), item.getActiveStage());
        }
        if (item.getCount()) {
            if (item.getCount() == item.getMaxCount()) {
                w->setOverlayColor({31,255,31});
            } else {
                w->setOverlayColor({220,220,220});
            }
            w->setOverlay(std::to_string(item.getCount()));
            w->setFont(_fontStore->getFont(DEFAULT_FONT_NAME,
                    FontStore::sizeFromData(DEFAULT_FONT_SIZE, item.getOverlayFontSize())));
        } else {
            auto s = item.getOverlay();
            w->setOverlay(s);
            if (!s.empty()) {
                w->setOverlayColor({220,220,220});
                w->setFont(_fontStore->getFont(DEFAULT_FONT_NAME,
                        FontStore::sizeFromData(DEFAULT_FONT_SIZE, item.getOverlayFontSize())));
            }
        }
    }
    updateLocations();
}

size_t TrackerView::addLayoutNodes(Container* container, const std::list<LayoutNode>& nodes, size_t depth)
{
    // This returns the number of added children
    
    size_t n=0;
    for (const auto& node: nodes) {
        if (addLayoutNode(container, node, depth)) n++;
    }
    return n;
}
bool TrackerView::addLayoutNode(Container* container, const LayoutNode& node, size_t depth)
{    
    // This returns true if a child was added, false otherwise
    
    if (depth>63) {
        fprintf(stderr, "Layout depth too high!\n");
        return false;
    }
    
#ifdef DEBUG_LAYOUT_TREE
    printf("%sadding layout node '%s'\n", std::string(depth*2,' ').c_str(),
                                          node.getType().c_str());
#endif
    
    const auto& children = node.getChildren();
    
    if (node.getType() == "container" || node.getType() == "tab") {
        Container *w = new SimpleContainer(0,0,container->getWidth(),container->getHeight());
        w->setGrow(1,1); // required at the moment -- TODO: make this depend on children
        if (!node.getBackground().empty()) w->setBackground(node.getBackground());
        addLayoutNodes(w, children, depth+1);
        container->addChild(w);
    }
    else if (node.getType() == "dock") {
        int width=container->getWidth()/2;   // NOTE: this is not ideal, but a
        int height=container->getHeight()/2; // preset is required at the moment
        auto sz = node.getSize();
        auto maxSz = node.getMaxSize();
        if (sz.x>0) width = sz.x;
        if (sz.y>0) height = sz.y;
        
        Dock *w = new Dock(0,0,width,height);
        if (maxSz.x > 0) w->setMaxSize( {maxSz.x, w->getMaxWidth()} );
        if (maxSz.y > 0) w->setMaxSize( {w->getMaxHeight(), maxSz.y} );
        if (!node.getBackground().empty()) w->setBackground(node.getBackground());
        for (const auto& childnode: children) {
            if (addLayoutNode(w, childnode, depth+1)) {
                w->setDock(-1, childnode.getDock());
            }
        }
        container->addChild(w);
    }
    else if (node.getType() == "array") {
        // TODO: set spacing from layout.json
        Container *w;
        if (node.getOrientation() == LayoutNode::Orientation::HORIZONTAL) {
            HBox *hbox = new HBox(0,0,0,0);
            hbox->setPadding(0);
            hbox->setSpacing(0);
            w = hbox;
        } else {
            VBox *vbox = new VBox(0,0,0,0);
            vbox->setPadding(0);
            vbox->setSpacing(0);
            w = vbox;
        }
        auto dfltMargin = (dynamic_cast<HBox*>(container) || dynamic_cast<VBox*>(container))
                ? LayoutNode::Spacing{0,0,0,0} : LayoutNode::Spacing{5,5,5,5};
        const auto& m = node.getMargin(dfltMargin);
        w->setMargin({m.left, m.top, m.right, m.bottom});
        if (!node.getBackground().empty()) w->setBackground(node.getBackground());
        addLayoutNodes(w, children, depth+1);
        container->addChild(w);
    }
    else if (node.getType() == "tabbed") {
        // NOTE: the way this widget works is that we have
        //       1 child per tab + 1 title per tab
        //       + a private hbox for tab buttons
        Tabs *w = new Tabs(0,0,container->getWidth(),container->getHeight(),_font);
        w->setGrow(1,1); // required at the moment -- TODO: make this depend on children
        if (!node.getBackground().empty()) w->setBackground(node.getBackground());
        for (const auto& childnode: children) {
            if (addLayoutNode(w, childnode, depth+1)) {
                auto& name = childnode.getHeader();
                w->setTabName(-1, name);
                if (std::find(_activeTabs.begin(), _activeTabs.end(), name) != _activeTabs.end()) {
                    w->setActiveTab(name);
                }
            }
        }
        container->addChild(w);
        _tabs.push_back(w);
        _tracker->onUiHint += { this, [this,w](void* s, const std::string& name, const std::string& value) {
            if (name == "ActivateTab") {
                w->setActiveTab(value);
            } else if (name =="reset") {
                w->setActiveTab(0);
            }
        }};
    }
    else if (node.getType() == "group") {
        Container *w = new Group(0,0,0,0,_font,node.getHeader());
        if (!node.getBackground().empty()) w->setBackground(node.getBackground());
        addLayoutNodes(w, children, depth+1);
        container->addChild(w);
    }
    else if (node.getType() == "item") {
        auto sz = node.getItemSize();
        auto maxSz = node.getMaxSize();
        if (maxSz.x>0 && sz.x<1) sz.x = maxSz.x;
        if (maxSz.y>0 && sz.y<1) sz.y = maxSz.y;
        Item *w = makeItem(0,0,sz.x,sz.y, _tracker->getItemByCode(node.getItem()));
        w->setImageAlignment(str2itemHalign(node.getItemHAlignment()), str2itemValign(node.getItemVAlignment()));
        if (maxSz.x > 0) w->setMaxSize( {maxSz.x, w->getMaxWidth()} );
        if (maxSz.y > 0) w->setMaxSize( {w->getMaxHeight(), maxSz.y} );
        if (!node.getBackground().empty()) w->setBackground(node.getBackground());
        // FIXME: this is a dirty work-around. make auto-layout better
        w->setMinSize(w->getAutoSize());
        if (w->getMaxWidth()>0 && w->getMaxWidth() < w->getMinWidth()) {
            int calculatedHeight = w->getMaxWidth()*w->getAutoHeight()/w->getAutoWidth(); // keep aspect ratio
            w->setMinSize({w->getMaxWidth(), calculatedHeight});
        }
        if (w->getMaxHeight()>0 && w->getMaxHeight() < w->getMinHeight()) {
            int calculatedWidth = w->getMaxHeight()*w->getAutoWidth()/w->getAutoHeight(); // keep aspect ratio
            w->setMinSize({calculatedWidth, w->getMaxHeight()});
        }
        const auto& m = node.getMargin({0,0,0,0});
        w->setMargin({m.left, m.top, m.right, m.bottom});
        container->addChild(w);
    }
    else if (node.getType() == "itemgrid") {
        const auto& rows = node.getRows();
        size_t rowCount = rows.size();
        size_t colCount = 0;
        for (const auto& row: rows) { 
            if (row.size()>colCount) colCount = row.size();
        }
        Container *w = new SimpleContainer(0,0,container->getWidth(),container->getHeight()); // TODO: itemgrid
        const auto& m = node.getMargin();
        w->setMargin({m.left, m.top, m.right, m.bottom});
        if (!node.getBackground().empty()) w->setBackground(node.getBackground());
        int y=0;
        auto sz = node.getItemSize();
        if (sz.x == -1 && sz.y == -1) {
            sz.x = 32; sz.y = 32; // default
        }
        if (sz.x < 1 || sz.y < 1) {
            fprintf(stderr, "*** NOT IMPLEMENTED ***\n");
            sz.x = 32; sz.y = 32; // fall-back
        }
        LayoutNode::Size sp = node.getItemMargin();
        int halign = node.getHAlignment() == "center" ? 0 : node.getHAlignment() == "right" ? 1 : -1;
        Label::HAlign itemHalign = str2itemHalign(node.getItemHAlignment());
        Label::VAlign itemValign = str2itemValign(node.getItemHAlignment());
        int maxX = 0, maxY = 0;
        for (const auto& row: rows) {
            int x = 0;
            int offx = 0;
            if (row.size() != colCount && halign != -1) {
                offx = (int)(colCount-row.size())*(sz.x+2*sp.x);
                if (halign == 0) offx /= 2;
            }
            for (const auto& item: row) {
                Item *iw = makeItem(offx+x*sz.x+(x*2+1)*sp.x, y*sz.y+(y*2+1)*sp.y, sz.x, sz.y, _tracker->getItemByCode(item));
                iw->setImageAlignment(itemHalign, itemValign);
                w->addChild(iw);
                if (iw->getLeft() + iw->getWidth() > maxX) maxX = iw->getLeft() + iw->getWidth();
                if (iw->getTop() + iw->getHeight() > maxY) maxY = iw->getTop() + iw->getHeight();
                x++;
            }
            y++;
        }
        //w->setSize({maxX, maxY});
        w->setSize({(int)colCount*(sz.x+2*sp.x),(int)rowCount*(sz.y+2*sp.y)});
        // FIXME: this is a dirty work-around. make auto-layout better
        w->setMinSize(w->getSize());
        container->addChild(w);
    }
    else if (node.getType() == "map") {
        // NOTE: we actually can have more than one map reference here. no clue how this is supposed to work yet
        // if maps is empty, add all maps
        // maybe we want to add a hboc, vbox or dock in case of multiple maps?
        auto maps = node.getMaps();
        if (maps.empty()) maps = _tracker->getMapNames();
        if (maps.empty()) return false;
        Container* originalContainer = container;
        Dock *hbox = nullptr; // FIXME: hbox does not work with maps yet
        if (maps.size()>1) {
            //hbox = new HBox(0,0,0,0);
            hbox = new Dock(0,0,0,0, true);
            container = hbox;
        }
        for (const auto& mapname : maps)
        {
            const auto& map = _tracker->getMap(mapname);
            const auto& f = map.getImage();
            std::string s;
            _tracker->getPack()->ReadFile(f, s);
            MapWidget *w = new MapWidget(0,0,0,0, s.c_str(),s.length());
            _maps[mapname].push_back(w);
            w->setQuality(2);
            if (!node.getBackground().empty()) w->setBackground(node.getBackground());
            w->setGrow(1,1);
            w->setMinSize({200,200});
            w->setLocationSize(map.getLocationSize());
            w->setLocationBorder(map.getLocationBorderTickness());
            for (const auto& pair : _tracker->getMapLocations(mapname)) {
                int state = 0; // this is done later to avoid calling into Lua
                w->addLocation(pair.first, pair.second.getX(), pair.second.getY(), state);
            }
            container->addChild(w);
            
            w->onLocationHover += { this, [this](void* sender, std::string locid, int absX, int absY) {
                // TODO: move this somewhere to increase readabil
                // TODO: if tooltip is the same, just move it
                MapWidget *map = static_cast<MapWidget*>(sender);
                if (_mapTooltip) {
                    // remove references to old tooltip
                    _mapTooltipOwner = nullptr;
                    if (_hoverChild == _mapTooltip) _hoverChild = nullptr;
                    _mapTooltip->onMouseLeave -= this;
                    // remove child from container
                    removeChild(_mapTooltip);
                    // removing a child may fire a signal, so we need to make sure not to double-free
                    if (_mapTooltip) {
                        // run destructor after removing reference since delete may also fire
                        auto tmp = _mapTooltip;
                        _mapTooltip = nullptr;
                        delete tmp;
                    }
                }
                
                // create tooltip
                _mapTooltipOwner = map;
                _mapTooltipName = locid;
                _mapTooltipPos = {absX,absY};
                _mapTooltip = makeMapTooltip(locid, absX-_absX-TOOL_OFF, absY-_absY-TOOL_OFF);
                // fix up position
                int mapLeft = map->getAbsLeft()-_absX; // FIXME: this is not really a good solution
                int mapTop = map->getAbsTop()-_absY;
                // TODO: include window size in this calculation? (content can be bigger than window)
                if (_mapTooltip->getLeft() + _mapTooltip->getWidth() > mapLeft + map->getWidth()) {
                    if ((_mapTooltip->getLeft() + _mapTooltip->getWidth()) - (mapLeft + map->getWidth()) <= TOOL_MAX_DISPLACEMENT)
                        // displace so it fits
                        _mapTooltip->setLeft(mapLeft + map->getWidth() - _mapTooltip->getWidth());
                    else {
                        // move it to left of cursor
                        _mapTooltip->setLeft(absX-_absX-_mapTooltip->getWidth()+TOOL_OFF);
                        if (_mapTooltip->getLeft() < mapLeft)
                            _mapTooltip->setLeft(mapLeft);
                    }
                }
                if (_mapTooltip->getTop() + _mapTooltip->getHeight() > mapTop + map->getHeight()) {
                    if ((_mapTooltip->getTop() + _mapTooltip->getHeight()) - (mapTop + map->getHeight()) <= TOOL_MAX_DISPLACEMENT)
                        // displace so it fits
                        _mapTooltip->setTop(mapTop + map->getHeight() - _mapTooltip->getHeight());
                    else {
                        // move it to top of cursor
                        _mapTooltip->setTop(absY-_absY-_mapTooltip->getHeight()+TOOL_OFF);
                        if (_mapTooltip->getTop() < mapTop)
                            _mapTooltip->setTop(mapTop);
                    }
                }
                // add tooltip to widgets
                addChild(_mapTooltip);
                // since we do not update mouse enter/leave in addChild (yet), do it by hand
                _hoverChild = _mapTooltip;
                map->onMouseLeave.emit(sender);
                
                _mapTooltip->onMouseLeave += { this, [this](void* sender) {
                    if (_mapTooltip) {
                        _mapTooltipOwner = nullptr;
                        // removing a child may fire a signal, so we need to make sure not to double-free
                        removeChild(_mapTooltip);
                        if (_mapTooltip) {
                            // run destructor after removing reference since delete may also fire
                            auto tmp = _mapTooltip;
                            _mapTooltip = nullptr;
                            delete tmp;
                        }
                    }
                }};
            }};
        }
        if (hbox) {
            //hbox->relayout();
            originalContainer->addChild(hbox);
        }
    }
    else if (node.getType() == "recentpins") {
        // TODO: recentpins
#ifdef NDEBUG
        Label *w = new Label(0,0,0,0, nullptr, "");
#else
        Label *w = new Label(0,0,0,0, _font, "PINS");
        w->setBackground({0x3f,0x00,0x00,0xff}); // TEST
#endif
        if (!node.getBackground().empty()) w->setBackground(node.getBackground());
        w->setGrow(1,1);
        container->addChild(w);
    }
    else if (node.getType() == "layout") {
        _layoutRefs.push_back(node.getKey());
        return addLayoutNode(container, _tracker->getLayout(node.getKey()), depth+1);
    }
    else if (node.getType() == "text") {
        Label *w = new Label(0,0,0,0, _font, node.getText());
        Label::HAlign halign = Label::HAlign::LEFT;
        Label::VAlign valign = Label::VAlign::TOP;
        if (node.getHAlignment() == "center")
            halign = Label::HAlign::CENTER;
        else if (node.getHAlignment() == "right")
            halign = Label::HAlign::RIGHT;
        if (node.getVAlignment() == "center")
            valign = Label::VAlign::MIDDLE;
        else if (node.getVAlignment() == "bottom")
            valign = Label::VAlign::BOTTOM;
        w->setTextAlignment(halign, valign);
        w->setSize(w->getAutoSize());
        w->setGrow(1,0);
        container->addChild(w);
    }
    else {
        fprintf(stderr, "Invalid or unsupported node: %s\n", node.getType().c_str());
        return false;
    }
    
    return true;
}

void TrackerView::setSize(Size size)
{
    //if (size == _size) return;
    // TODO: resize on next frame?
    SimpleContainer::setSize(size);
}

void TrackerView::addChild(Widget* child)
{
    if (!child) return;
    // TODO: move margin to SimpleContainer?
    child->setPosition({child->getLeft() + child->getMargin().left, child->getTop() + child->getMargin().top});
    SimpleContainer::addChild(child);
}

Container* TrackerView::makeMapTooltip(const std::string& locid, int x, int y)
{
    // TODO: move MapTooltip to mapwidget?
    bool compact = true;
    
    Container* tooltip;
    {
        auto tmp = new VBox(x, y, 100, 100);
        tmp->setPadding(2*TOOL_OFF);
        tmp->setSpacing(TOOL_OFF);
        tooltip = tmp;
    }
    auto& loc = _tracker->getLocation(locid);
    const auto& name = loc.getName();
    if (!name.empty()) {
        Label* lbl = new Label(0,0,0,0, _font, name);
        unsigned state = calculateLocationState(locid);
        if (state == 2) lbl->setTextColor({255,64,64}); // unreachable: red
        else if (state == 4 || state == 6) lbl->setTextColor({255,255,128}); // glitches required: yellow
        else if (state == 8 || state == 10) lbl->setTextColor({96,128,255}); // checkable: blue
        lbl->setTextAlignment(Label::HAlign::RIGHT, Label::VAlign::MIDDLE);
        lbl->setSize(lbl->getSize()||lbl->getMinSize()); // FIXME: this should not be neccessary
        lbl->setMinSize(lbl->getSize()||lbl->getMinSize());
        tooltip->addChild(lbl);
    }
    
    Container* sectionContainer;
    bool horizontalSections = false; //= compact;
    if (horizontalSections) {
        auto tmp = new HBox(0,0,0,0);
        tmp->setSpacing(2*TOOL_OFF);
        sectionContainer = tmp;
    } else {
        sectionContainer = tooltip;
    }
    
    for (const auto& sec : loc.getSections()) {
        if (!_tracker->isVisible(loc, sec)) continue;

        Container* c = horizontalSections ? new VBox(0,0,0,0) : tooltip;

        int reachable = _tracker->isReachable(loc, sec);
        bool cleared = false; // not implemented

        if (!sec.getName().empty()) {
            Label* lbl = new Label(0,0,0,0, _smallFont, sec.getName());
            if (cleared) lbl->setTextColor({128,128,128}); // cleared: grey; TODO: use array for colors?
            else if (reachable==0) lbl->setTextColor({255,32,32}); // unreachable: red
            else if (reachable==2) lbl->setTextColor({255,255,32}); // glitches required: yellow
            else if (reachable==3) lbl->setTextColor({48,64,255}); // checkable: blue
            lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
            lbl->setSize(lbl->getSize()||lbl->getMinSize()); // FIXME: this should not be neccessary
            lbl->setMinSize(lbl->getSize()||lbl->getMinSize());
            c->addChild(lbl);
        }
        if (sec.getItemCount() > 0 || sec.getHostedItems().size()>0) {
            HBox* hbox = new HBox(0,0,0,0);
            int itemcount = sec.getItemCount();
            int looted = sec.getItemCleared();
            for (int i=0; i<itemcount; i++) {
                bool opened = compact ? looted>=itemcount : i<=looted;
                Item *w = makeLocationIcon(0,0,32,32, locid, sec, opened, compact);
                hbox->addChild(w);
                if (compact) break;
            }
            for (const auto& item: sec.getHostedItems()) {
                Item *w = makeItem(0,0,32,32, _tracker->getItemByCode(item));
                w->setMinSize(w->getSize()); // FIXME: this is a dirty work-around
                hbox->addChild(w);
            }
            hbox->relayout(); // FIXME: this should not be neccessary
            c->addChild(hbox); 
        }
        if (c != sectionContainer)
            sectionContainer->addChild(c);
    }
    if (sectionContainer != tooltip)
        tooltip->addChild(sectionContainer);
    //tooltip->relayout(); // FIXME: this should not be neccessary
    
    tooltip->setMinSize(tooltip->getMinSize() || TOOL_MIN_SIZE);
    tooltip->setBackground({0x00,0x00,0x00,0xbf});
    tooltip->setSize(tooltip->getMinSize());
    tooltip->setGrow(0,0);
    
    return tooltip;
}


int TrackerView::calculateLocationState(const std::string& locid)
{
    auto& loc = _tracker->getLocation(locid);
    bool hasReachable = false;
    bool hasUnreachable = false;
    bool hasGlitchedReachable = false;
    bool hasCheckable = false;
    bool hasVisible = false;

    for (const auto& sec: loc.getSections()) {
        if (_tracker->isVisible(loc, sec)) {
            hasVisible = true;
        } else {
            continue;
        }

        int itemCount = sec.getItemCount();
        if (itemCount>0 && sec.getItemCleared()>=itemCount) continue;
        else if (itemCount<1) { // check hosted items
            bool match = false;
            for (const auto& code: sec.getHostedItems()) {
                if (!_tracker->ProviderCountForCode(code)) {
                    match = true;
                    break;
                }
            }
            if (!match) continue;
        }

        int reachable = _tracker->isReachable(loc, sec);
        if (reachable==1) {
            hasReachable = true;
        } else if (reachable==0) {
            hasUnreachable = true;
        } else if (reachable==3) {
            hasCheckable = true;
        } else {
            hasGlitchedReachable = true;
        }
        if (hasReachable && hasUnreachable && hasGlitchedReachable && hasCheckable) break;
    }
    if (!hasVisible) return -1;
    uint8_t res = (hasCheckable?(1<<3):0) |
                  (hasGlitchedReachable?(1<<2):0) |
                  (hasUnreachable?(1<<1):0) |
                  (hasReachable?(1<<0):0);
    return res;
}


} // namespace
