#ifndef _UI_DEFAULTTRACKERWINDOW_H
#define _UI_DEFAULTTRACKERWINDOW_H

#include "trackerwindow.h"
#include "loadpackwidget.h"

namespace Ui {

class DefaultTrackerWindow : public TrackerWindow {
public:
    using FONT = Window::FONT;
    
    DefaultTrackerWindow(const char *title, SDL_Surface* icon=nullptr, const Position& pos=WINDOW_DEFAULT_POSITION, const Size& size={0,0});
    virtual ~DefaultTrackerWindow();
    
    virtual void setTracker(Tracker *tracker, const std::string& layout="tracker_default") override;
    virtual void setAutoTrackerState(AutoTracker::State state) override;
    virtual void setSize(Size size) override;
    virtual void showOpen();
    virtual void hideOpen();
    
    Signal<const std::string&,const std::string&> onPackSelected;
    
protected:
    ImageButton *_btnLoad = nullptr;
    ImageButton *_btnReload = nullptr;
    ImageButton *_btnImport = nullptr;
    ImageButton *_btnExport = nullptr;
    ImageButton *_btnBroadcast = nullptr;
    ImageButton *_btnPackSettings = nullptr;
    Label *_lblAutoTracker = nullptr;
    Label *_lblTooltip = nullptr;
    LoadPackWidget *_loadPackWidget = nullptr;
    AutoTracker::State _autoTrackerState = AutoTracker::State::Disconnected;
};

} // namspace Ui

#endif /* _UI_DEFAULTTRACKERWINDOW_H */

