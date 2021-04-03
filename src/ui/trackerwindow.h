#ifndef _UI_TRACKERWINDOW_H
#define _UI_TRACKERWINDOW_H

#include <string>
#include "../uilib/window.h"
#include "../uilib/label.h"
#include "../uilib/image.h"
#include "../uilib/imagebutton.h"
#include "trackerview.h"
#include "../core/tracker.h"
#include "../core/autotracker.h"

namespace Ui {

class TrackerWindow : public Window {
public:
    using FONT = Window::FONT;
    
    TrackerWindow(const char *title, SDL_Surface* icon=nullptr, const Position& pos=WINDOW_DEFAULT_POSITION, const Size& size={0,0});
    virtual ~TrackerWindow();
    
    virtual void setTracker(Tracker* tracker);
    virtual void setSize(Size size) override;
    
    virtual void setAutoTrackerState(AutoTracker::State state);
    
    std::list< std::pair<std::string,std::string> > getHints() const;
    
    Signal<const std::string&> onMenuPressed;
    
    static const std::string MENU_LOAD;
    static const std::string MENU_RELOAD;
    static const std::string MENU_BROADCAST;
    static const std::string MENU_PACK_SETTINGS;
    
    virtual void setCenterPosition(const Position& pos) { _centerPos = pos; }
    
protected:
    Container *_menu;
    TrackerView *_view;
    bool _resizeScheduled;
    Size _resizeSize;
    FONT _smallFont = nullptr;
    bool _rendered = false;
    std::string _baseTitle;
    Position _centerPos = {-1,-1};
    
    virtual void render(Renderer renderer, int offX, int offY) override;
    virtual void setTracker(Tracker* tracker, const std::string& layout);
};

} // namespace Ui

#endif // _UI_TRACKERWINDOW_H
