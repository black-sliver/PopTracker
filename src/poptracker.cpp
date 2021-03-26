#include "poptracker.h"
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <SDL2/SDL_image.h>
#include "ui/trackerwindow.h"
#include "ui/broadcastwindow.h"
#include "core/luaitem.h"
#include "core/imagereference.h"
#include "core/fileutil.h"
#include "core/assets.h"
#include "core/jsonutil.h"
#include "core/statemanager.h"
using nlohmann::json;


PopTracker::PopTracker(int argc, char** argv)
{
    _ui = new Ui::Ui(APPNAME);
    _ui->onWindowDestroyed += {this, [this](void*, Ui::Window *win) {
        if (win == _broadcast) {
            _broadcast = nullptr;
        }
        if (win == _settings) {
            _settings = nullptr;
        }
        if (win == _win) {
            _win = nullptr;
        }
    }};
    
    Pack::addSearchPath("packs"); // current directory
    
    std::string cwdPath = getCwd();
    std::string documentsPath = getDocumentsPath();
    std::string homePath = getHomePath();
    std::string appPath = getAppPath();
    
    if (!homePath.empty() && homePath != "." && homePath != cwdPath) {
        Pack::addSearchPath(os_pathcat(homePath,"PopTracker","packs")); // default user packs
        Assets::addSearchPath(os_pathcat(homePath,"PopTracker","assets")); // default user overrides
    }
    if (!documentsPath.empty() && documentsPath != "." && documentsPath != cwdPath) {
        Pack::addSearchPath(os_pathcat(documentsPath,"PopTracker","packs")); // alternative user packs
        Pack::addSearchPath(os_pathcat(documentsPath,"EmoTracker","packs")); // "old" packs
    }
    if (!appPath.empty() && appPath != "." && appPath != cwdPath) {
        Pack::addSearchPath(os_pathcat(appPath,"packs")); // system packs
        Assets::addSearchPath(os_pathcat(appPath,"assets")); // system assets
    }
    
    StateManager::setDir(getConfigPath(APPNAME, "saves"));
}
PopTracker::~PopTracker()
{
    unloadTracker();
    _win = nullptr;
    delete _ui;
}

bool PopTracker::start()
{
    std::string config;
    json jConfig;
    if (readFile(getConfigPath(APPNAME, std::string(APPNAME)+".json"), config)) {
        jConfig = parse_jsonc(config);
    }
    
    Ui::Position pos = WINDOW_DEFAULT_POSITION;
    Ui::Size size = {0,0};
    
    // restore state from config
    if (jConfig.type() == json::value_t::object) {
        auto jWindow = jConfig["window"];
        if (jWindow.type() == json::value_t::object) {
            auto& jPos = jWindow["pos"];
            auto& jSize = jWindow["size"];
            if (jPos.type() == json::value_t::array) {
                pos.left = to_int(jPos[0],pos.left);
                pos.top = to_int(jPos[1],pos.top);
            }
            if (jSize.type() == json::value_t::array) {
                size.width = to_int(jSize[0],size.width);
                size.height = to_int(jSize[1],size.height);
            }
        }
        auto jPack = jConfig["pack"];
        if (jPack.type() == json::value_t::object) {
            std::string path = to_string(jPack["path"],"");
            std::string variant = to_string(jPack["variant"],"");
            if (!scheduleLoadTracker(path,variant))
            {
                printf("Could not load pack \"%s\"... fuzzy matching\n", path.c_str());
                // try to fuzzy match by uid and version if path is missing
                std::string uid = to_string(jPack["uid"],"");
                std::string version = to_string(jPack["version"],"");
                Pack::Info info = Pack::Find(uid, version);
                if (info.path.empty()) info = Pack::Find(to_string(jPack["uid"],""));
                if (!scheduleLoadTracker(info.path, variant)) {
                    printf("Could not load pack uid \"%s\" (\"%s\")\n", uid.c_str(), info.path.c_str());
                } else {
                    printf("found pack \"%s\"\n", info.path.c_str());
                }
            }
        }
    }
    
    auto icon = IMG_Load(asset("icon.png").c_str());
    _win = _ui->createWindow<Ui::DefaultTrackerWindow>("PopTracker", icon, pos, size);
    SDL_FreeSurface(icon);
    
    _win->onMenuPressed += { this, [this](void*, const std::string& item) {
        if (item == Ui::TrackerWindow::MENU_BROADCAST) {
            if (!_tracker) return;
            if (_broadcast) {
                _broadcast->Raise();
            } else {
                auto icon = IMG_Load(asset("icon.png").c_str());
                Ui::Position pos = _win->getPosition() + Ui::Size{0,32};
                _broadcast = _ui->createWindow<Ui::BroadcastWindow>("Broadcast", icon, pos);
                SDL_FreeSurface(icon);
                _broadcast->setTracker(_tracker);
                pos += Ui::Size{_win->getWidth()/2, _win->getHeight()/2 - 32};
                _broadcast->setCenterPosition(pos); // this will reposition the window after rendering
            }
        }
        if (item == Ui::TrackerWindow::MENU_PACK_SETTINGS)
        {
            if (!_tracker) return;
#ifndef __EMSCRIPTEN__ // no multi-window support (yet)
            if (_settings) {
                _settings->Raise();
            } else {
                auto icon = IMG_Load(asset("icon.png").c_str());
                Ui::Position pos = _win->getPosition() + Ui::Size{0,32};
                _settings = _ui->createWindow<Ui::SettingsWindow>("Settings", icon, pos);
                _settings->setBackground({255,0,255});
                SDL_FreeSurface(icon);
                _settings->setTracker(_tracker);
            }
#else
            // TODO: show settings inside main window with close-button
#endif
        }
        if (item == Ui::TrackerWindow::MENU_LOAD)
        {
            _win->showOpen();
        }
        if (item == Ui::TrackerWindow::MENU_RELOAD)
        {
#if 0
            if (!_pack) return;
            scheduleLoadTracker(_pack->getPath(), _pack->getVariant(), false);
#else
            if (!_tracker) return;
            StateManager::loadState(_tracker, _scriptHost, false, "reset");
#endif
            
        }
    }};
    
    _win->onPackSelected += {this, [this](void *s, const std::string& pack, const std::string& variant) {
        scheduleLoadTracker(pack, variant);
        _win->hideOpen();
    }};
    
    _frameTimer = std::chrono::steady_clock::now();
    _fpsTimer = std::chrono::steady_clock::now();
    return (_win != nullptr);
}

bool PopTracker::frame()
{
    if (_scriptHost) _scriptHost->autoTrack();
    
#define SHOW_FPS
#ifdef SHOW_FPS
    auto now = std::chrono::steady_clock::now();
    // time since last frame
    auto td = std::chrono::duration_cast<std::chrono::milliseconds>(now - _frameTimer).count();
    if (td > _maxFrameTime) _maxFrameTime = td;
    _frameTimer = now;
    // time since last fps display
    td = std::chrono::duration_cast<std::chrono::milliseconds>(now - _fpsTimer).count();
    if (td >= 5000) {
        unsigned f = _frames*1000; f/=td;
        printf("FPS:%4u (max %2dms)\n", f, _maxFrameTime);
        _frames = 0;
        _fpsTimer = now;
        _maxFrameTime = 0;
    }
#endif
    _frames++;
    bool res = _ui->render();
    
    if (!res) {
        // application is going to exit
        auto pos = _win->getOuterPosition();
        auto size = _win->getSize();
        auto disppos = _win->getPositionOnDisplay();
        _config["format_version"] = 1;
        if (_pack) _config["pack"] = { 
            {"path",_pack->getPath()},
            {"variant",_pack->getVariant()},
            {"uid",_pack->getUID()},
            {"version",_pack->getVersion()}
        };
        _config["window"] = { 
            {"pos", {pos.left,pos.top}},
            {"size",{size.width,size.height}},
            {"display", _win->getDisplay()},
            {"display_name", _win->getDisplayName()},
            {"display_pos", {disppos.left,disppos.top}}
        };
        std::string configDir = getConfigPath(APPNAME);
        mkdir_recursive(configDir.c_str());
        writeFile(os_pathcat(configDir, std::string(APPNAME)+".json"), (_config.dump()+"\n").c_str());
    }
    
    // load new tracker AFTER rendering a frame
    if (!_newTracker.empty()) {
        if (!loadTracker(_newTracker, _newVariant, _newTrackerLoadAutosave)) {
            // TODO: display error
        }
        _newTracker = "";
        _newVariant = "";
    }
    return res;
}

void PopTracker::unloadTracker()
{
    if (_tracker) {
        StateManager::saveState(_tracker, _scriptHost, true);
    }

    // remove references before deleting _tracker
    if (_win) _win->setTracker(nullptr);
    if (_broadcast) {
        _broadcast->setTracker(nullptr);
        _ui->destroyWindow(_broadcast);
        _broadcast = nullptr;
    }
    if (_settings) {
        _settings->setTracker(nullptr);
        _ui->destroyWindow(_settings);
        _settings = nullptr;
    }
#ifndef FIND_LUA_LEAKS // if we don't lua_close at exit, we can use valgrind to see if the allocated memory grew
    if (_L) lua_close(_L);
#endif
    delete _tracker;
    delete _scriptHost;
    delete _pack;
    _L = nullptr;
    _tracker = nullptr;
    _scriptHost = nullptr;
    _pack = nullptr;
}
bool PopTracker::loadTracker(const std::string& pack, const std::string& variant, bool loadAutosave)
{
    unloadTracker();
    
    _pack = new Pack(pack);
    _pack->setVariant(variant);
    
    _L = luaL_newstate();
    luaL_openlibs(_L);
    
    _tracker = new Tracker(_pack, _L);
    Tracker::Lua_Register(_L);
    _tracker->Lua_Push(_L);
    lua_setglobal(_L, "Tracker");
    
    _scriptHost = new ScriptHost(_pack, _L, _tracker);
    ScriptHost::Lua_Register(_L);
    _scriptHost->Lua_Push(_L);
    lua_setglobal(_L, "ScriptHost");
    
    LuaItem::Lua_Register(_L); // TODO: move this to Tracker or ScriptHost
    JsonItem::Lua_Register(_L); // ^
    LocationSection::Lua_Register(_L); // ^
    
    ImageReference::Lua_Register(_L);
    _imageReference.Lua_Push(_L);
    lua_setglobal(_L, "ImageReference");
    
    // because this is a good way to detect auto-tracking support
    Lua(_L).Push("Lua 5.3");
    lua_setglobal(_L, "_VERSION");
    
    Lua(_L).Push(VERSION_STRING);
    lua_setglobal(_L, "PopVersion");
    
    _win->setTracker(_tracker);
    if (auto at = _scriptHost->getAutoTracker()) {
        _win->setAutoTrackerState(at->getState());
        at->onStateChange += {this, [this](void*, AutoTracker::State state)
        {
            if (_win) _win->setAutoTrackerState(state);
        }};
    }
    
    // run pack's init script
    bool res = _scriptHost->LoadScript("scripts/init.lua");
    StateManager::saveState(_tracker, _scriptHost, false, "reset");
    if (loadAutosave) {
        // restore previous state
        StateManager::loadState(_tracker, _scriptHost, true);
    }
    return res;
}
bool PopTracker::scheduleLoadTracker(const std::string& pack, const std::string& variant, bool loadAutosave)
{
    // TODO: (load and) verify pack already?
    if (! pathExists(pack)) return false;
    _newTracker = pack;
    _newVariant = variant;
    _newTrackerLoadAutosave = loadAutosave;
    return true;
}

