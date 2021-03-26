#include "settingswindow.h"

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
}
void SettingsWindow::render(Renderer renderer, int offX, int offY)
{
    TrackerWindow::render(renderer, offX, offY);
}

} // namespace

