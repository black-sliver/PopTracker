#include "maptooltip.h"
#include "../uilib/label.h"
#include "../uilib/hbox.h"
#include "../uilib/vbox.h"
#include "../core/assets.h"
#include "../core/fileutil.h"
#include "../ui/trackerview.h"
#include <list>


namespace Ui {

#define _DEFAULT_SECTION_COLORS { \
    /* done (unused) */ \
    {0x80, 0x80, 0x80},\
    /* accessible */ \
    {0xff, 0xff, 0xff},\
    /* inaccessible */ \
    {0xcf, 0x10, 0x10},\
    /* glitches required */ \
    {0xff, 0xff, 0x20},\
    /* scoutable */ \
    {0x30, 0x40, 0xff},\
}

Widget::Color MapTooltip::StateColors[] = _DEFAULT_SECTION_COLORS;


static uint8_t mix(uint8_t v, uint8_t w, uint8_t weight)
{
    return ((uint32_t)w * (uint32_t)weight + (uint32_t)v * (255 - weight))/255;
}

Widget::Color MapTooltip::getSectionColor(AccessibilityLevel reachable, bool cleared)
{
    Widget::Color c;
    if (cleared) c = StateColors[0]; // cleared: grey (unused)
    else if (reachable == AccessibilityLevel::NONE) c = StateColors[2]; // unreachable: red
    else if (reachable == AccessibilityLevel::SEQUENCE_BREAK) c = StateColors[3]; // glitches required: yellow
    else if (reachable == AccessibilityLevel::INSPECT) c = StateColors[4]; // checkable: blue
    else c = StateColors[1]; // default: white
    c.r = mix(c.r, 0xff, 18);
    c.g = mix(c.g, 0xff, 18);
    c.b = mix(c.b, 0xff, 18);
    return c;
}


MapTooltip::MapTooltip(int x, int y, FONT font, FONT smallFont, int quality, Tracker* tracker, const std::string& locid,
        std::function<Item*(int, int, int, int, const std::string& code)> makeItem)
    : ScrollVBox(x, y, 100, 100)
{
    bool compact = true;
    _id = locid;

    setPadding(2*OFFSET);
    setSpacing(OFFSET);

    auto& loc = tracker->getLocation(locid);
    const auto& name = loc.getName();
    if (!name.empty()) {
        HBox* hbox = new HBox(0,0,0,0);
        Label* lbl = new Label(0,0,0,0, font, name);
        unsigned state = TrackerView::CalculateLocationState(tracker, locid);
        bool cleared = false; // not implemented below, so not using it here
        Widget::Color c;
        if (cleared) c = StateColors[0]; // cleared: grey (unused)
        else if (state == 2) c = StateColors[2]; // unreachable: red
        else if (state == 4 || state == 6) c = StateColors[3]; // glitches required: yellow
        else if (state == 8 || state == 10) c = StateColors[4]; // checkable: blue
        else c = StateColors[1]; // default: white
        c.r = mix(c.r, 0xff, 64);
        c.g = mix(c.g, 0xff, 64);
        c.b = mix(c.b, 0xff, 64);
        lbl->setTextColor(c);
        lbl->setTextAlignment(Label::HAlign::RIGHT, Label::VAlign::MIDDLE);
        lbl->setSize(lbl->getSize()||lbl->getMinSize()); // FIXME: this should not be necessary
        lbl->setMinSize(lbl->getSize()||lbl->getMinSize());
        hbox->addChild(lbl);
        addChild(hbox);
    }

    Container* sectionContainer;
    bool horizontalSections = false; //= compact;
    if (horizontalSections) {
        auto tmp = new HBox(0,0,0,0);
        tmp->setSpacing(2*OFFSET);
        sectionContainer = tmp;
    } else {
        sectionContainer = this;
    }

    for (const auto& ogSec : loc.getSections()) {
        const auto& sec = ogSec.getRef().empty() ? ogSec : tracker->getLocationSection(ogSec.getRef());
        _sectionLocations.push_back({});
        auto& locationItems = _sectionLocations.back();
        bool visible = tracker->isVisible(loc, sec);
        auto reachable = tracker->isReachable(loc, sec);
        bool cleared = false; // not implemented

        std::list<std::string> hostedItems;
        for (const auto& hostedItem: sec.getHostedItems()) {
            // skip missing/undefined ones
            if (tracker->getItemByCode(hostedItem).getType() != BaseItem::Type::NONE)
                hostedItems.push_back(hostedItem);
        }

        if (sec.getItemCount() > 0 || hostedItems.size() > 0) {
            Container* container = horizontalSections ? (Container*)new VBox(0,0,0,0) : (Container*)this;

            const std::string& ogName = ogSec.getName();
            const std::string& name = ogName.empty() ? sec.getName() : ogName;
            if (!name.empty()) {
                Label* lbl = new Label(0,0,0,0, smallFont, name);
                _sectionLabels.push_back(lbl);
                lbl->setVisible(visible);
                Widget::Color c = getSectionColor(reachable, cleared);
                lbl->setTextColor(c);
                lbl->setTextAlignment(Label::HAlign::LEFT, Label::VAlign::MIDDLE);
                lbl->setSize(lbl->getSize()||lbl->getMinSize()); // FIXME: this should not be necessary
                lbl->setMinSize(lbl->getSize()||lbl->getMinSize());
                container->addChild(lbl);
            } else {
                _sectionLabels.push_back(nullptr);
            }

            HBox* hbox = new HBox(0,0,0,0);
            _sectionContainers.push_back(hbox);
            int itemcount = sec.getItemCount();
            int looted = sec.getItemCleared();

            // add location items
            for (int i=0; i<itemcount; i++) {
                bool opened = compact ? looted>=itemcount : i<=looted;
                Item *w = MakeLocationIcon(0,0,32,32, font, quality, tracker, sec.getParentID(), sec, opened, compact);
                locationItems.push_back(w);
                hbox->addChild(w);
                if (compact)
                    break;
            }
            // add hosted items
            for (const auto& item: hostedItems) {
                Item *w = makeItem(0,0,32,32, item);
                _items[tracker->getItemByCode(item).getID()].push_back(w);
                w->setMinSize(w->getSize()); // FIXME: this is a dirty work-around
                hbox->addChild(w);
                if (sec.getClearAsGroup()) {
                    // override onClick to clear everything
                    w->onClick.clear();
                    w->onClick += {w, [tracker, name, ogName, locid](void*, int x, int y, int btn) {
                        auto& loc = tracker->getLocation(locid);
                        for (auto& ogSec: loc.getSections()) {
                            if (ogSec.getName() != ogName)
                                continue;
                            auto& sec = ogSec.getRef().empty() ? ogSec : tracker->getLocationSection(ogSec.getRef());
                            if (ogSec.getName() != name && sec.getName() != name)
                                continue;
                            // clear all hosted items
                            std::list<std::string> codes = sec.getHostedItems();
                            for (const auto& item: codes) {
                                tracker->changeItemState(tracker->getItemByCode(item).getID(),
                                        btn == BUTTON_RIGHT ? BaseItem::Action::Secondary : BaseItem::Action::Primary);
                            }
                            // clear regular items
                            if (btn == BUTTON_RIGHT)
                                sec.unclearItem();
                            else
                                sec.clearItem();
                            break;
                        }
                    }};
                }
            }
            hbox->setVisible(visible);
            container->addChild(hbox);
            if (container != sectionContainer)
                sectionContainer->addChild(container);
        } else {
            _sectionLabels.push_back(nullptr);
            _sectionContainers.push_back(nullptr);
        }
    }
    if (sectionContainer != this)
        addChild(sectionContainer);

    setMinSize(getMinSize() || MIN_SIZE);
    setBackground({0x00,0x00,0x00,0xbf});
    setSize(getAutoSize());
    setGrow(0,0);
}


Item* MapTooltip::MakeLocationIcon(int x, int y, int width, int height, FONT font, int quality,
        Tracker* tracker, const std::string& locid, const LocationSection& sec, bool opened, bool compact)
{
    std::string fClosed, fOpened;
    std::string sClosed, sOpened;

    fClosed = sec.getClosedImage();
    tracker->getPack()->ReadFile(fClosed, sClosed);
    if (sClosed.empty()) {
        readFile(asset("closed.png"), sClosed); // fallback/default icon
    }

    fOpened = sec.getOpenedImage();
    tracker->getPack()->ReadFile(fOpened, sOpened);
    if (sOpened.empty()) {
        readFile(asset("open.png"), sOpened); // fallback/default icon
    }

    Item *w = new Item(x, y, width, height, font);
    w->setQuality(quality);
    w->addStage(0,0, sClosed.c_str(), sClosed.length(), fClosed); // TODO: +img_mods
    w->addStage(1,0, sOpened.c_str(), sOpened.length(), fOpened); // TODO: +img_mods
    w->setStage(opened?1:0,0);
    w->setMinSize(w->getSize()); // FIXME: this is a dirty work-around
    w->setMaxSize(w->getSize());
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
    w->onClick += {w, [w,locid,name,compact,tracker](void*,int x, int y, int btn) {
        if (!compact && btn == BUTTON_RIGHT && w->getStage1()<1)
            return;
        if (!compact && btn != BUTTON_RIGHT && w->getStage1()>0)
            return;
        auto& loc = tracker->getLocation(locid);
        for (auto& sec: loc.getSections()) {
            if (sec.getName() != name) continue;
            // check hosted items with chests if "clear_as_group" is set
            if (sec.getClearAsGroup()) {
                std::list<std::string> codes = sec.getHostedItems();
                for (const auto& code: codes) {
                    const auto& item = tracker->getItemByCode(code);
                    tracker->changeItemState(item.getID(),
                            btn == BUTTON_RIGHT ? BaseItem::Action::Secondary : BaseItem::Action::Primary);
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

void MapTooltip::update(Tracker* tracker, std::function<void(Item*, const BaseItem&)> updateItem)
{
    for (auto& pair: _items) {
        const auto& item = tracker->getItemById(pair.first);
        for (auto w: pair.second) {
            updateItem(w, item);
        }
    }
    auto& loc = tracker->getLocation(_id);
    int i = 0;
    bool relayoutRequired = false;
    for (const auto& ogSec : loc.getSections()) {
        const auto& sec = ogSec.getRef().empty() ? ogSec : tracker->getLocationSection(ogSec.getRef());
        bool visible = tracker->isVisible(loc, sec);

        Label* lbl = _sectionLabels[i];
        Container* container = _sectionContainers[i];
        if (container) {
            if (visible != container->getVisible()) {
                container->setVisible(visible);
                relayoutRequired = true;
            }
        }
        if (lbl) {
            if (visible != lbl->getVisible()) {
                lbl->setVisible(visible);
                relayoutRequired = true;
            }
            auto reachable = tracker->isReachable(loc, sec);
            bool cleared = false; // not implemented below, so not using it here
            Widget::Color c = getSectionColor(reachable, cleared);
            lbl->setTextColor(c);
            // NOTE: currently there will always be only one _sectionLocation since compact is always true
        }
        bool compact = true;
        if (!_sectionLocations[i].empty()) {
            int itemcount = sec.getItemCount() ;
            int looted = sec.getItemCleared();
            bool opened = compact ? looted>=itemcount : i<=looted;
            Item* w = _sectionLocations[i].front();
            if (itemcount!=1 && itemcount>looted) {
                w->setOverlay(std::to_string(itemcount - looted));
            } else {
                w->setOverlay("");
            }
            w->setStage(opened?1:0,0);
        }
        i++;
    }
    if (relayoutRequired)
        relayout();
}

const std::string& MapTooltip::getLocationID() const
{
    return _id;
}

} // namespace Ui
