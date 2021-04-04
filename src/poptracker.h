#ifndef _POPTRACKER_H
#define _POPTRACKER_H

#include "app.h"
#include "uilib/ui.h"
#include "ui/defaulttrackerwindow.h"
#include "ui/broadcastwindow.h"
#include "ui/settingswindow.h"
#include "luaglue/lua.h"
#include "core/pack.h"
#include "core/tracker.h"
#include "core/scripthost.h"
#include "core/imagereference.h"
#include <chrono>
#include <json/json.hpp>

class PopTracker final : public App {
private:

    Ui::Ui *_ui = nullptr;
    Ui::DefaultTrackerWindow *_win = nullptr;
    Ui::BroadcastWindow *_broadcast = nullptr;
    Ui::SettingsWindow *_settings = nullptr;
    nlohmann::json _config;
    
    lua_State* _L = nullptr;
    Tracker *_tracker = nullptr;
    ScriptHost *_scriptHost = nullptr;
    Pack *_pack = nullptr;
    ImageReference _imageReference;
    
    unsigned _frames = 0;
    unsigned _maxFrameTime = 0;
    std::chrono::steady_clock::time_point _fpsTimer;
    std::chrono::steady_clock::time_point _frameTimer;
    
    std::string _newTracker;
    std::string _newVariant;
    bool _newTrackerLoadAutosave = false;
    
    bool loadTracker(const std::string& pack, const std::string& variant, bool loadAutosave=true);
    bool scheduleLoadTracker(const std::string& pack, const std::string& variant, bool loadAutosave=true);
    void unloadTracker();
    
public:
    PopTracker(int argc, char** argv);
    virtual ~PopTracker();
    
    static constexpr const char APPNAME[] = "PopTracker";
    static constexpr const char VERSION_STRING[] = "0.11.0";
    
protected:
    virtual bool start();
    virtual bool frame();
};

#endif // _POPTRACKER_H
