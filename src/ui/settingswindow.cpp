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
        _view->onItemTooltip += {this, [this, tracker](void*, const std::string& id) {
            if (id.empty()) {
                showTooltip(nullptr);
            } else {
                auto& item = tracker->getItemById(id);
                if (item.getName().empty())
                    showTooltip(nullptr);
                else
                    showTooltip(item.getName());
            }
        }};
    }
}

void SettingsWindow::render(Renderer renderer, int offX, int offY)
{
    TrackerWindow::render(renderer, offX, offY);
}

} // namespace

