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
    
    virtual void setTracker(Tracker* tracker) = 0;
    virtual void setSize(Size size) override;
    
    virtual void setAutoTrackerState(int index, AutoTracker::State state, const std::string& name, const std::string& subname);
    
    std::list< std::pair<std::string,std::string> > getHints() const;

    virtual void setCenterPosition(const Position& pos) { _centerPos = pos; }

    void setHideClearedLocations(bool hide);
    void unsetHideClearedLocations();
    void setHideUnreachableLocations(bool hide);
    void unsetHideUnreachableLocations();

    Signal<const std::string&, int> onMenuPressed;
    
    static const std::string MENU_LOAD;
    static const std::string MENU_RELOAD;
    static const std::string MENU_LOAD_STATE;
    static const std::string MENU_SAVE_STATE;
    static const std::string MENU_BROADCAST;
    static const std::string MENU_PACK_SETTINGS;
    static const std::string MENU_TOGGLE_AUTOTRACKER;
    static const std::string MENU_CYCLE_AUTOTRACKER;

protected:
    Container *_menu;
    TrackerView *_view;
    bool _resizeScheduled;
    Size _resizeSize;
    bool _rendered = false;
    std::string _baseTitle;
    Position _centerPos = {-1,-1};
    bool _overrideUnreachableVisibility = false;
    bool _hideUnreachableLocations = false;
    bool _overrideClearedVisibility = false;
    bool _hideClearedLocations = false;

    virtual void render(Renderer renderer, int offX, int offY) override;
    virtual void setTracker(Tracker* tracker, const std::string& layout);
};

} // namespace Ui

#endif // _UI_TRACKERWINDOW_H
