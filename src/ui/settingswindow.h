#ifndef _UI_SETTINGSWINDOW_H
#define _UI_SETTINGSWINDOW_H

#include "trackerwindow.h"

namespace Ui {

class SettingsWindow : public TrackerWindow {
public:
    using FONT = Window::FONT;
    
    SettingsWindow(const char *title, SDL_Surface* icon=nullptr, const Position& pos=WINDOW_DEFAULT_POSITION, const Size& size={0,0});
    virtual ~SettingsWindow();
    
    virtual void setTracker(Tracker *tracker) override;
    virtual void render(Renderer renderer, int offX, int offY) override;
};

} // namespace Ui

#endif // _UI_SETTINGSWINDOW_H
