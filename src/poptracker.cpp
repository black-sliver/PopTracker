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
#include "core/log.h"
#include <tinyfiledialogs.h>
#include "http/http.h"
#ifdef _WIN32
#include <windows.h>
#endif
using nlohmann::json;


PopTracker::PopTracker(int argc, char** argv)
{
    std::string config;
    std::string configFilename = getConfigPath(APPNAME, std::string(APPNAME)+".json");
    if (readFile(configFilename, config)) {
        _config = parse_jsonc(config);
        _oldConfig = _config;
    }

    if (_config["log"].type() != json::value_t::boolean)
        _config["log"] = false;
    if (_config["software_renderer"].type() != json::value_t::boolean)
        _config["software_renderer"] = false;

    std::string logFilename = getConfigPath(APPNAME, "log.txt");
    if (!_config["log"]) {
        // disable logging, leave note
#if 0
        if (Log::RedirectStdOut(logFilename)) {
            printf("%s %s\n", APPNAME, VERSION_STRING);
        }
        printf("Logging disabled. Exit application, change '\"log\":false' "
                "to '\"log\":true' in\n  '%s'\n  and restart to enable.\n",
                configFilename.c_str());
        Log::UnredirectStdOut();
#endif
    } else {
        // enable logging
        printf("Logging to %s\n", logFilename.c_str());
        if (Log::RedirectStdOut(logFilename)) {
            printf("%s %s\n", APPNAME, VERSION_STRING);
        }
    }

#ifndef WITHOUT_UPDATE_CHECK
    if (_config.find("check_for_updates") == _config.end()) {
        auto res = tinyfd_messageBox("PopTracker",
                "Check for PopTracker updates on start?",
                "yesno", "question", 1);
        _config["check_for_updates"] = (res != 0);
    }
#endif

    if (_config.find("add_emo_packs") == _config.end()) {
#if defined _WIN32 || defined WIN32
        auto res = tinyfd_messageBox("PopTracker",
                "Add Documents\\EmoTracker\\packs to the search path?",
                "yesno", "question", 1);
        _config["add_emo_packs"] = (res != 0);
#else
        _config["add_emo_packs"] = false;
#endif
    }

    _asio = new asio::io_service();
    HTTP::certfile = asset("cacert.pem"); // https://curl.se/docs/caextract.html
#ifndef WITHOUT_UPDATE_CHECK
    if (_config["check_for_updates"]) {
        printf("Checking for update...\n");
        std::string s;
        const std::string url = "https://api.github.com/repos/black-sliver/PopTracker/releases?per_page=8";
        // .../releases/latest would be better, but does not return pre-releases
        if (!HTTP::GetAsync(*_asio, url,
                [this](int code, const std::string& content)
                {
                    if (code == 200) {
                        try {
                            auto j = json::parse(content);
                            std::string version;
                            std::list<std::string> assets;
                            std::string url;
                            for (size_t i=0; i<j.size(); i++) {
                                auto rls = j[i];
                                version = rls["tag_name"].get<std::string>();
                                if ((version[0]=='v' || version[0]=='V') && isNewer(version)) {
                                    version = version.substr(1);
                                    url = rls["html_url"].get<std::string>();
                                    for (auto val: rls["assets"]) {
                                        assets.push_back(val["browser_download_url"].get<std::string>());
                                    }
                                    break;
                                } else {
                                    version.clear();
                                }
                            }
                            if (!version.empty())
                                updateAvailable(version, url, assets);
                            else
                                printf("Update: already up to date\n");
                        } catch (...) {
                            fprintf(stderr, "Update: error parsing json\n");
                        }
                    } else {
                        fprintf(stderr, "Update: server returned code %d\n", code);
                    }
                },
                [](...)
                {
                    fprintf(stderr, "Update: error getting response\n");
                }))
        {
            fprintf(stderr, "Update: error starting request\n");
        }
    }
#endif

    if (!_config["fps_limit"].is_number())
        _config["fps_limit"] = DEFAULT_FPS_LIMIT;

    if (_config["export_file"].is_string() && _config["export_uid"].is_string()) {
        _exportFile = _config["export_file"];
        _exportUID = _config["export_uid"];
    }
    if (_config["export_dir"].is_string())
        _exportDir = _config["export_dir"];

    saveConfig();

    _ui = new Ui::Ui(APPNAME, _config["software_renderer"]?true:false);
    _ui->setFPSLimit(_config["fps_limit"].get<unsigned>());
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
        if (_config["add_emo_packs"]) {
            Pack::addSearchPath(os_pathcat(documentsPath,"EmoTracker","packs")); // "old" packs
        }
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
    delete _asio;
    _win = nullptr;
    delete _ui;
}

bool PopTracker::start()
{
    Ui::Position pos = WINDOW_DEFAULT_POSITION;
    Ui::Size size = {0,0};
    
    // restore state from config
    if (_config.type() == json::value_t::object) {
        auto jWindow = _config["window"];
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
        auto jPack = _config["pack"];
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
        if (item == Ui::TrackerWindow::MENU_TOGGLE_AUTOTRACKER)
        {
            if (!_scriptHost || !_scriptHost->getAutoTracker()) return;
            auto at = _scriptHost->getAutoTracker();
            if (!at) return;
            if (at->getState() == AutoTracker::State::Disabled) {
                at->enable();
                _autoTrackerDisabled = false;
            }
            else if (at->getState() != AutoTracker::State::Disabled) {
                at->disable();
                _autoTrackerDisabled = true;
            }
        }
        if (item == Ui::TrackerWindow::MENU_SAVE_STATE)
        {
            if (!_tracker) return;
            if (!_pack) return;
            std::string lastName;
            if (_exportFile.empty() || _exportUID != _pack->getUID()) {
                lastName = _pack->getGameName() + ".json";
                if (!_exportDir.empty()) lastName = os_pathcat(_exportDir, lastName);
            } else {
                lastName = _exportFile;
            }

            const char* ext[] = {"*.json"};
            auto filename = tinyfd_saveFileDialog("Save State",
                    lastName.c_str(), 1, ext, "JSON Files");
            if (filename && *filename) {
                if (StateManager::saveState(_tracker, _scriptHost,
                        _win->getHints(), true, filename, true))
                {
                    _exportFile = filename;
                    _exportUID = _pack->getUID();
                    _exportDir = os_dirname(_exportFile);
                }
                else
                {
                    tinyfd_messageBox("PopTracker", "Error saving state!",
                            "ok", "error", 0);
                }
            }
        }
        if (item == Ui::TrackerWindow::MENU_LOAD_STATE)
        {
            std::string lastName = _exportFile.empty() ? (_exportDir + OS_DIR_SEP) : _exportFile;
            const char* ext[] = {"*.json"};
            auto filename = tinyfd_openFileDialog("Load State",
                    lastName.c_str(), 1, ext, "JSON Files", 0);
            std::string s;
            if (!readFile(filename, s)) {
                fprintf(stderr, "Error reading state file: %s\n", filename);
                tinyfd_messageBox("PopTracker", "Could not read state file!",
                        "ok", "error", 0);
                return;
            }
            json j;
            try {
                j = parse_jsonc(s);
            } catch (...) {
                fprintf(stderr, "Error parsing state file: %s\n", filename);
                tinyfd_messageBox("PopTracker", "Selected file is not valid json!",
                        "ok", "error", 0);
                return;
            }
            auto jPack = j["pack"];
            if (!jPack.is_object() || !jPack["uid"].is_string() || !jPack["variant"].is_string()) {
                fprintf(stderr, "Json is not a state file: %s\n", filename);
                tinyfd_messageBox("PopTracker", "Selected file is not a state file!",
                        "ok", "error", 0);
                return;
            }
            if (_pack->getUID() != jPack["uid"] || _pack->getVariant() != jPack["variant"]) {
                auto packInfo = Pack::Find(jPack["uid"]);
                if (packInfo.uid != jPack["uid"]) {
                    std::string msg = "Could not find pack: " + sanitize_print(jPack["uid"]) + "!";
                    fprintf(stderr, "%s\n", msg.c_str());
                    tinyfd_messageBox("PopTracker", msg.c_str(), "ok", "error", 0);
                    return;
                }
                bool variantMatch = false;
                for (auto& v: packInfo.variants) {
                    if (v.variant == jPack["variant"]) {
                        variantMatch = true;
                        break;
                    }
                }
                if (!variantMatch) {
                    std::string msg = "Pack " + sanitize_print(jPack["uid"]) +
                            " does not have requested variant "
                            + sanitize_print(jPack["variant"]);
                    fprintf(stderr, "%s\n", msg.c_str());
                    tinyfd_messageBox("PopTracker", msg.c_str(), "ok", "error", 0);
                    return;
                }
                if (!loadTracker(packInfo.path, jPack["variant"], false)) {
                    tinyfd_messageBox("PopTracker", "Error loading pack!",
                            "ok", "error", 0);
                }
            }
            if (!StateManager::loadState(_tracker, _scriptHost, true, filename, true)) {
                tinyfd_messageBox("PopTracker", "Error loading state!",
                        "ok", "error", 0);
            }
        }
    }};
    
    _win->onPackSelected += {this, [this](void *s, const std::string& pack, const std::string& variant) {
        printf("Pack selected: %s:%s\n", pack.c_str(), variant.c_str());
        if (!scheduleLoadTracker(pack, variant)) {
            fprintf(stderr, "Error scheduling load of tracker/pack!");
            // TODO: show error
        }
        _win->hideOpen();
    }};
    
    _frameTimer = std::chrono::steady_clock::now();
    _fpsTimer = std::chrono::steady_clock::now();
    return (_win != nullptr);
}

bool PopTracker::frame()
{
    if (_asio) _asio->poll();
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
        auto pos = _win->getPlacementPosition();
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
        _config["export_file"] = _exportFile;
        _config["export_uid"] = _exportUID;
        _config["export_dir"] = _exportDir;
        saveConfig();
    }
    
    // load new tracker AFTER rendering a frame
    if (!_newTracker.empty()) {
        printf("Loading Tracker %s:%s!\n", _newTracker.c_str(), _newVariant.c_str()); fflush(stdout);
        if (!loadTracker(_newTracker, _newVariant, _newTrackerLoadAutosave)) {
            fprintf(stderr, "Error loading tracker/pack!\n"); fflush(stderr);
            // TODO: display error
        } else {
            printf("Tracker loaded!\n"); fflush(stdout);
        }
        _newTracker = "";
        _newVariant = "";
    }
    return res;
}

void PopTracker::unloadTracker()
{
    if (_tracker) {
        StateManager::saveState(_tracker, _scriptHost, _win->getHints(), true);
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
    delete _scriptHost;
    delete _tracker;
    delete _pack;
    _L = nullptr;
    _tracker = nullptr;
    _scriptHost = nullptr;
    _pack = nullptr;
}
bool PopTracker::loadTracker(const std::string& pack, const std::string& variant, bool loadAutosave)
{
    printf("Cleaning up...\n");
    unloadTracker();
    
    printf("Loading Pack...\n");
    _pack = new Pack(pack);
    printf("Selecting Variant...\n");
    _pack->setVariant(variant);
    
    printf("Creating Lua State...\n");
    _L = luaL_newstate();
    if (!_L) {
        fprintf(stderr, "Error creating Lua State!\n");
        delete _pack;
        _pack = nullptr;
        return false;
    }
    printf("Loading Lua libs...\n");
    luaL_openlibs(_L);
    
    printf("Loading Tracker...\n");
    _tracker = new Tracker(_pack, _L);
    printf("Registering in Lua...\n");
    Tracker::Lua_Register(_L);
    _tracker->Lua_Push(_L);
    lua_setglobal(_L, "Tracker");
    
    printf("Creating Script Host...\n");
    _scriptHost = new ScriptHost(_pack, _L, _tracker);
    printf("Registering in Lua...\n");
    ScriptHost::Lua_Register(_L);
    _scriptHost->Lua_Push(_L);
    lua_setglobal(_L, "ScriptHost");
    if (_scriptHost->getAutoTracker() && _autoTrackerDisabled)
        _scriptHost->getAutoTracker()->disable();
    
    printf("Registering classes in Lua...\n");
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
    
    printf("Updating UI\n");
    _win->setTracker(_tracker);
    if (auto at = _scriptHost->getAutoTracker()) {
        _win->setAutoTrackerState(at->getState());
        at->onStateChange += {this, [this](void*, AutoTracker::State state)
        {
            if (_win) _win->setAutoTrackerState(state);
        }};
    }
    
    // run pack's init script
    printf("Running scripts/init.lua\n");
    bool res = _scriptHost->LoadScript("scripts/init.lua");
    // save reset-state
    StateManager::saveState(_tracker, _scriptHost, _win->getHints(), false, "reset");
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

void PopTracker::updateAvailable(const std::string& version, const std::string& url, const std::list<std::string> assets)
{
    std::string ignoreData;
    std::string ignoreFilename = getConfigPath(APPNAME, "ignored-versions.json");
    json ignore;
    if (readFile(ignoreFilename, ignoreData)) {
        ignore = parse_jsonc(ignoreData);
        if (ignore.is_array()) {
            if (std::find(ignore.begin(), ignore.end(), version) != ignore.end()) {
                printf("Update: %s is in ignore list\n", version.c_str());
                return;
            }
        } else {
            ignore = json::array();
        }
    }
    
    std::string msg = "Update to PopTracker " + version + " available. Download?";
    if (tinyfd_messageBox("PopTracker", msg.c_str(), "yesno", "question", 1))
    {
#if defined _WIN32 || defined WIN32
        ShellExecuteA(nullptr, "open", url.c_str(), NULL, NULL, SW_SHOWDEFAULT);
#else
        auto pid = fork();
        if (pid == -1) {
            fprintf(stderr, "Update: fork failed\n");
        } else if (pid == 0) {
#if defined __APPLE__ || defined MACOS
            const char* exe = "open";
#else
            const char* exe = "xdg-open";
#endif
            execlp(exe, exe, url.c_str(), (char*)nullptr);
            std::string msg = "Could not launch browser!\n";
            msg += exe;
            msg += ": ";
            msg += strerror(errno);
            fprintf(stderr, "Update: %s\n", msg.c_str());
            tinyfd_messageBox("PopTracker", msg.c_str(), "ok", "error", 0);
            exit(0);
        }
#endif
    }
    else {
        msg = "Skip version " + version + "?";
        if (tinyfd_messageBox("PopTracker", msg.c_str(), "yesno", "question", 1))
        {
            printf("Update: ignoring %s\n", version.c_str());
            ignore.push_back(version);
            writeFile(ignoreFilename, ignore.dump(0));
        }
    }
}

bool PopTracker::isNewer(const Version& v)
{
    // returns true if v is newer than current PopTracker
    return v>Version(VERSION_STRING);
}

bool PopTracker::saveConfig()
{
    bool res = true;
    if (_oldConfig != _config) {
        std::string configDir = getConfigPath(APPNAME);
        mkdir_recursive(configDir.c_str());
        res = writeFile(os_pathcat(configDir, std::string(APPNAME)+".json"), _config.dump(4)+"\n");
        if (res) _oldConfig = _config;
    }
    return res;
}
