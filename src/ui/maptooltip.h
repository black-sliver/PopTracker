#ifndef _UI_MAPTOOLTIP_H
#define _UI_MAPTOOLTIP_H

#include "../uilib/scrollvbox.h"
#include "../core/tracker.h"
#include "../uilib/label.h"
#include "item.h"
#include "../core/location.h"
#include <functional>


namespace Ui {

class MapTooltip : public ScrollVBox {
public:
    using FONT = Label::FONT;

    MapTooltip(int x, int y, FONT font, FONT smallFont, int quality, Tracker* tracker, const std::string& locid,
        std::function<Item*(int, int, int, int, const std::string& code)> makeItem);

    static Item* MakeLocationIcon(int x, int y, int width, int height, FONT font, int quality,
        Tracker* tracker, const std::string& locid, const LocationSection& sec, bool opened, bool compact);

    const std::string& getLocationID() const;

    void update(Tracker* tracker, std::function<void(Item*, const BaseItem&)> updateItem);

    static constexpr Size MIN_SIZE={ 32,32 };
    static constexpr int OFFSET=2;

    static Widget::Color StateColors[5];
    static Widget::Color getSectionColor(AccessibilityLevel reachable, bool cleared);

private:
    std::string _id;
    std::map<std::string, std::list<Item*>> _items;
    std::vector<Label*> _sectionLabels;
    std::vector<Container*> _sectionContainers;
    std::vector<std::list<Item*>> _sectionLocations;
};


} // namespace UI

#endif // _UI_MAPTOOLTIP_H

