#include "broadcastwindow.h"

namespace Ui {

BroadcastWindow::BroadcastWindow(const char* title, SDL_Surface* icon, const Position& pos, const Size& size)
    : TrackerWindow(title, icon, pos, {size.width < 1 ? 100 : size.width, size.height < 1 ? 100:size.height})
{
    if (size.width<1 || size.height<1) resize({100,100});
}

BroadcastWindow::~BroadcastWindow()
{
}

void BroadcastWindow::setTracker(Tracker* tracker)
{
    TrackerWindow::setTracker(tracker, "tracker_broadcast");
    setBackground({1,0,0});
}
void BroadcastWindow::render(Renderer renderer, int offX, int offY)
{
    TrackerWindow::render(renderer, offX, offY);
}

} // namespace

