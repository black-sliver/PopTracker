#include "settingswindow.h"
#include "../uilib/tooltip.h"

namespace Ui {

SettingsWindow::SettingsWindow(const char* title, SDL_Surface* icon, const Position& pos, const Size& size)
    : TrackerWindow(title, icon, pos, size)
{
    if (size.width<1 || size.height<1) resize({100,100});
}

SettingsWindow::~SettingsWindow()
{
}

void SettingsWindow::setTracker(Tracker* tracker)
{
    setBackground({0x7f,0x7f,0x7f});
    TrackerWindow::setTracker(tracker, "settings_popup");
    if (_view && tracker) {
        // Future improvement: widgets could store a ref to the parent and create the tooltips themselves
        _view->onItemTooltip += {this, [this, tracker](void*, const std::string& itemid) {
            if (itemid.empty()) {
                showTooltip(nullptr);
                auto& item = tracker->getItemById(_tooltipItemId);
                item.onChange -= this;
                _tooltipItemId.clear();
            } else {
                auto& item = tracker->getItemById(itemid);
                const auto& text = (item.getBaseItem().empty() || item.getState()) ?
                    item.getCurrentName() : tracker->getItemByCode(item.getBaseItem()).getCurrentName();
                if (text.empty()) {
                    showTooltip(nullptr);
                    _tooltipItemId.clear();
                } else {
                    showTooltip(text);
                    _tooltipItemId = itemid;
                    item.onChange += {this, [this, tracker, itemid](void* sender) {
                        const auto& item = tracker->getItemById(itemid);
                        const auto& text = (item.getBaseItem().empty() || item.getState()) ?
                            item.getCurrentName() : tracker->getItemByCode(item.getBaseItem()).getCurrentName();
                        if (auto tooltip = dynamic_cast<Tooltip*>(_tooltip)) {
                            tooltip->setText(text);
                            tooltip->setSize(tooltip->getMinSize());
                        }
                    }};
                }
            }
        }};
    }
}

void SettingsWindow::render(Renderer renderer, int offX, int offY)
{
    TrackerWindow::render(renderer, offX, offY);
}

} // namespace

