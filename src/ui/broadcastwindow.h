#ifndef _UI_BROADCASTWINDOW_H
#define _UI_BROADCASTWINDOW_H

#include "trackerwindow.h"

namespace Ui {

class BroadcastWindow : public TrackerWindow {
public:
    using FONT = Window::FONT;
    
    BroadcastWindow(const char *title, SDL_Surface* icon=nullptr, const Position& pos=WINDOW_DEFAULT_POSITION, const Size& size={0,0});
    virtual ~BroadcastWindow();
    
    virtual void setTracker(Tracker *tracker) override;
    virtual void render(Renderer renderer, int offX, int offY) override;
};

} // namespace Ui

#endif // _UI_BROADCASTWINDOW_H
