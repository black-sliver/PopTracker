#include "poptracker.h"
#include <luaglue/lua_include.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include "ui/trackerwindow.h"
#include "ui/broadcastwindow.h"
#include "uilib/dlg.h"
#include "core/luaitem.h"
#include "core/imagereference.h"
#include "core/fileutil.h"
#include "core/assets.h"
#include "core/jsonutil.h"
#include "core/statemanager.h"
#include "core/log.h"
#include "http/http.h"
#include "ap/archipelago.h"
#include <luaglue/luaenum.h>
#include "luasandbox/require.h"
#include "ui/maptooltip.h"
#ifdef _WIN32
#include <windows.h>
#endif
using nlohmann::json;
using Ui::Dlg;


const Version PopTracker::VERSION = Version(APP_VERSION_TUPLE);


enum HotkeyID {
    HOTKEY_TOGGLE_VISIBILITY = 1,
    HOTKEY_RELOAD,
    HOTKEY_FORCE_RELOAD,
    HOTKEY_TOGGLE_SPLIT_COLORS,
    HOTKEY_SHOW_BROADCAST,
    HOTKEY_SHOW_HELP,
};

static char _globalStoreKey = 'k';
static const uintptr_t globalStoreIndex = (uintptr_t)&_globalStoreKey;
static char _globalPopKey = 'k';
static const uintptr_t globalPopIndex = (uintptr_t)&_globalPopKey;

int PopTracker::global_index(lua_State *L)
{
    lua_rawgeti(L, LUA_REGISTRYINDEX, (lua_Integer)globalStoreIndex);
    lua_pushvalue(L, -2);
    lua_rawget(L, -2);
    return 1;
}

int PopTracker::global_newindex(lua_State *L)
{
    bool toStore = false;
    if (lua_isstring(L, -2)) {
        // filter out special keys
        const char* key = lua_tostring(L, -2);
        if (strcmp(key, "DEBUG") == 0) {
            lua_rawgeti(L, LUA_REGISTRYINDEX, (lua_Integer)globalPopIndex);
            PopTracker* pop = static_cast<PopTracker*>(lua_touserdata(L, -1));
            lua_pop(L, 1); // pop
            if (!pop) {
                luaL_error(L, "Lua state not initialized correctly!");
            } else if (lua_isboolean(L, -1) || lua_isnil(L, -1)) {
                if (lua_toboolean(L, -1)) {
                    for (const char* flag: PopTracker::ALL_DEBUG_FLAGS)
                        pop->_debugFlags.insert(flag);
                } else {
                    pop->_debugFlags.clear();
                }
            } else if (lua_istable(L, -1)) {
                pop->_debugFlags.clear();
                lua_pushnil(L);
                while (lua_next(L, -2) != 0) {
                    pop->_debugFlags.insert(lua_tostring(L, -1));
                    lua_pop(L, 1); // value
                }
            } else if (lua_isstring(L, -1)) {
                pop->_debugFlags.clear();
                pop->_debugFlags.insert(lua_tostring(L, -1));
            } else {
                luaL_error(L, "Invalid assignment to global DEBUG");
            }
            toStore = true;
        }
    }
    if (toStore) {
        // store into separate store so we receive updates
        lua_rawgeti(L, LUA_REGISTRYINDEX, (lua_Integer)globalStoreIndex);
        lua_insert(L, -3);
        lua_rawset(L, -3);
    } else {
        // store into global object for speed optimization
        lua_rawset(L, -3);
    }
    return 0;
}

void PopTracker::global_wrap(lua_State *L, PopTracker* self)
{
    lua_pushglobaltable(L);
    lua_createtable(L, 0, 2);
    lua_pushcfunction(L, global_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, global_newindex);
    lua_setfield(L,-2, "__newindex");
    lua_setmetatable(L, -2);
    lua_pop(L,1);
    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, (lua_Integer)globalStoreIndex);
    lua_pushlightuserdata(L, self);
    lua_rawseti(L, LUA_REGISTRYINDEX, (lua_Integer)globalPopIndex);
}

static json windowToJson(Ui::Window* win)
{
    auto pos = win->getPlacementPosition();
    auto size = win->getSize();
    auto disppos = win->getPositionOnDisplay();

    // detect invalid positioning and size
    if (pos.left <= -32768 || pos.left >= 32767 ||
            pos.top <= -32768 || pos.top >= 32767)
        return nullptr;
    if (size.width < 1 || size.height < 1)
        return nullptr;

    return {
        {"pos", {pos.left,pos.top}},
        {"size",{size.width,size.height}},
        {"display", win->getDisplay()},
        {"display_name", win->getDisplayName()},
        {"display_pos", {disppos.left,disppos.top}}
    };
}

PopTracker::PopTracker(int argc, char** argv, bool cli, const json& args)
{
    _args = args;

    std::string appPath = getAppPath();
    if (!appPath.empty() && fileExists(os_pathcat(appPath, "portable.txt"))) {
        _isPortable = true;
    } else {
        _isPortable = false;  // make sure
    }

    std::string config;
    std::string configFilename = getConfigPath(APPNAME, std::string(APPNAME)+".json", _isPortable);
    if (readFile(configFilename, config)) {
        _config = parse_jsonc(config);
        _oldConfig = _config;
    }

    std::string colorsData;
    std::string colorsFilename = getConfigPath(APPNAME, "colors.json", _isPortable);
    if (readFile(colorsFilename, colorsData)) {
        _colors = parse_jsonc(colorsData);
        if (_colors.is_object()) {
            auto& stateColors = _colors["MapWidget.StateColors"];
            if (stateColors.is_array()) {
                ssize_t i = 0;
                for (auto& v: stateColors) {
                    if (!v.is_string()) {
                        fprintf(stderr, "Error: value not a string in colors.json\n");
                        break;
                    }
                    if (i >= countOf(Ui::MapWidget::StateColors)) break;
                    Ui::MapWidget::StateColors[i] = v.get<std::string>();
                    if (i == 0 || i == 2) // not changing touch [1] reachable: white
                        Ui::MapTooltip::StateColors[i] = Ui::MapWidget::StateColors[i];
                    if (i == 4)
                        Ui::MapTooltip::StateColors[3] = Ui::MapWidget::StateColors[i];
                    if (i == 8)
                        Ui::MapTooltip::StateColors[4] = Ui::MapWidget::StateColors[i];
                    i++;
                }
            } else if (!stateColors.is_null()) {
                fprintf(stderr, "Warning: invalid 'MapWidget.StateColors' in colors.json\n");
            }
        } else {
            fprintf(stderr, "Error: expected object at top level of colors.json\n");
        }
    }

    if (_config["log"].type() != json::value_t::boolean)
        _config["log"] = false;
    if (_config["software_renderer"].type() != json::value_t::boolean)
        _config["software_renderer"] = false;
    if (_config["enable_screensaver"].type() != json::value_t::boolean)
        _config["enable_screensaver"] = true;
    if (_config["ignore_hidpi"].type() != json::value_t::boolean)
        _config["ignore_hidpi"] = false;

    if (!cli) {
        // enable logging
        if (_config["log"]) {
            std::string logFilename = getConfigPath(APPNAME, "log.txt", _isPortable);
            printf("Logging to %s\n", logFilename.c_str());
            if (Log::RedirectStdOut(logFilename)) {
                printf("%s %s\n", APPNAME, VERSION_STRING);
            }
        }
#if defined _WIN32 || defined WIN32
        if (_config["ignore_hidpi"] == true)
            SetProcessDPIAware();
#endif

#ifndef WITHOUT_UPDATE_CHECK
        if (!Dlg::hasGUI()) {
            printf("Dialog helper not available! "
                    "Please install which and zenity, kdialog, matedialog, qarma, xdialog or python + tkinter.\n");
        }
        else if (_config.find("check_for_updates") == _config.end()) {
            auto res = Dlg::MsgBox("PopTracker",
                    "Check for PopTracker updates on start?",
                    Dlg::Buttons::YesNo, Dlg::Icon::Question);
            _config["check_for_updates"] = (res == Dlg::Result::Yes);
        }
#endif

        if (_config.find("add_emo_packs") == _config.end()) {
#if defined _WIN32 || defined WIN32
            auto res = Dlg::MsgBox("PopTracker",
                    "Add Documents\\EmoTracker\\packs to the search path?",
                    Dlg::Buttons::YesNo, Dlg::Icon::Question);
            _config["add_emo_packs"] = (res == Dlg::Result::Yes);
#else
            _config["add_emo_packs"] = false;
#endif
        }
    }

    int dnt = -1;
    auto dntIt = _config.find("do_not_track");
    if (dntIt != _config.end() && dntIt.value().is_boolean()) {
        dnt = dntIt.value().get<bool>() ? 1 : 0;
    } else if (dntIt != _config.end() && dntIt.value().is_number()) {
        dnt = dntIt.value().get<int>();
    } else {
        _config["do_not_track"] = nullptr;
    }

    if (dnt > 0) // only support 1 and null for now
        _httpDefaultHeaders.push_back("DNT: 1");

    if (!_config["fps_limit"].is_number())
        _config["fps_limit"] = DEFAULT_FPS_LIMIT;

    if (!_config["software_fps_limit"].is_number())
        _config["software_fps_limit"] = DEFAULT_SOFTWARE_FPS_LIMIT;

    if (_config["export_file"].is_string() && _config["export_uid"].is_string()) {
        _exportFile = pathFromUTF8(_config["export_file"]);
        _exportUID = _config["export_uid"];
    }

    if (_config["export_dir"].is_string())
        _exportDir = pathFromUTF8(_config["export_dir"]);

    if (_config["at_uri"].is_string())
        _atUri = _config["at_uri"];

    if (_config["at_slot"].is_string())
        _atSlot = _config["at_slot"];

    if (_config["usb2snes"].is_null())
        _config["usb2snes"] = "";

    if (_config["split_map_locations"].is_boolean())
        Ui::MapWidget::SplitRects = _config["split_map_locations"];

    if (_config["override_rule_exec_limit"].is_number_unsigned()) {
        if (_config["override_rule_exec_limit"] > std::numeric_limits<int>::max())
            _config["override_rule_exec_limit"] = std::numeric_limits<int>::max();
        Tracker::setExecLimit(_config["override_rule_exec_limit"]);
    } else if (!_config["override_rule_exec_limit"].is_null())
        _config["override_rule_exec_limit"] = nullptr; // clear invalid value

    auto debugIt = _config.find("debug");
    if (debugIt == _config.end()) {
        _config["debug"] = nullptr;
    } else if (debugIt.value().is_boolean()) {
        if (debugIt.value().get<bool>()) {
            for (const char* flag: ALL_DEBUG_FLAGS)
                _defaultDebugFlags.insert(flag);
        }
    } else if (debugIt.value().is_array()) {
        try {
            _defaultDebugFlags = debugIt.value().get<std::set<std::string>>();
        } catch (...) {}
    }
    _debugFlags = _defaultDebugFlags;

    saveConfig();

    _ui = nullptr; // UI init moved to start()

    Pack::addSearchPath("packs"); // current directory
    Pack::addOverrideSearchPath("user-override");

    std::string cwdPath = getCwd();
    std::string documentsPath = getDocumentsPath();
    std::string homePath = getHomePath();

    std::string homePopTrackerPath = os_pathcat(homePath, "PopTracker");
    _homePackDir = os_pathcat(homePopTrackerPath, "packs");
    _appPackDir = os_pathcat(appPath, "packs");

    if (!homePath.empty()) {
        Pack::addSearchPath(_homePackDir); // default user packs
        if (!_isPortable) {
            // default user overrides
            std::string homeUserOverrides = os_pathcat(homePopTrackerPath, "user-override");
            if (dirExists(homePopTrackerPath))
                mkdir_recursive(homeUserOverrides);
            Pack::addOverrideSearchPath(homeUserOverrides);
            Assets::addSearchPath(os_pathcat(homePopTrackerPath, "assets"));
        }
    }
    if (!documentsPath.empty() && documentsPath != ".") {
        std::string documentsPopTrackerPath = os_pathcat(documentsPath, "PopTracker");
        Pack::addSearchPath(os_pathcat(documentsPopTrackerPath, "packs")); // alternative user packs
        if (!_isPortable) {
            std::string documentsUserOverrides = os_pathcat(documentsPopTrackerPath, "user-override");
            Pack::addOverrideSearchPath(documentsUserOverrides);
            if (dirExists(documentsPopTrackerPath))
                mkdir_recursive(documentsUserOverrides);
        }
        if (_config.value<bool>("add_emo_packs", false)) {
            Pack::addSearchPath(os_pathcat(documentsPath, "EmoTracker", "packs")); // "old" packs
        }
    }

    if (!appPath.empty() && appPath != "." && appPath != cwdPath) {
        Pack::addSearchPath(_appPackDir); // system packs
        Pack::addOverrideSearchPath(os_pathcat(appPath, "user-override")); // portable/system overrides
        Assets::addSearchPath(os_pathcat(appPath, "assets")); // system assets
    }

    _asio = new asio::io_service();
    HTTP::certfile = asset("cacert.pem"); // https://curl.se/docs/caextract.html

    _packManager = new PackManager(_asio, getConfigPath(APPNAME, "", _isPortable), _httpDefaultHeaders);
    // TODO: move repositories to config?
    _packManager->addRepository("https://raw.githubusercontent.com/black-sliver/PopTracker/packlist/community-packs.json");
    // NOTE: signals are connected later to allow gui and non-gui interaction

    StateManager::setDir(getConfigPath(APPNAME, "saves", _isPortable));
}

PopTracker::~PopTracker()
{
    unloadTracker();
    delete _packManager;
    delete _asio;
    _win = nullptr;
    delete _ui;
}

bool PopTracker::start()
{
    Ui::Position pos = WINDOW_DEFAULT_POSITION;
    Ui::Size size = {0,0};

#ifndef WITHOUT_UPDATE_CHECK
    if (_config.value<bool>("check_for_updates", false)) {
        printf("Checking for update...\n");
        std::string s;
        const std::string url = "https://api.github.com/repos/black-sliver/PopTracker/releases?per_page=8";
        // .../releases/latest would be better, but does not return pre-releases
        auto requestHeaders = _httpDefaultHeaders;
        if (!HTTP::GetAsync(*_asio, url, requestHeaders,
                [this](int code, const std::string& content, HTTP::Headers)
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

    _ui = new Ui::Ui(APPNAME, _config["software_renderer"] ? true : false);
    _ui->setFPSLimit(_config["fps_limit"].get<unsigned>(), _config["software_fps_limit"].get<unsigned>());
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
    _ui->onHotkey += {this, [this](void*, const Ui::Ui::Hotkey& hotkey) {
        if (hotkey.id == HOTKEY_TOGGLE_VISIBILITY) {
            // until packs provide a default, we just toggle between "unset" and "hidden"
            auto it = _config.find("hide_cleared_locations");
            if (it == _config.end() || !it.value().is_boolean() || it.value() == false) {
                _config["hide_cleared_locations"] = true;
                _config["hide_unreachable_locations"] = true;
                if (_win) _win->setHideClearedLocations(true);
                if (_win) _win->setHideUnreachableLocations(true);
                if (_broadcast) _broadcast->setHideClearedLocations(true);
                if (_broadcast) _broadcast->setHideUnreachableLocations(true);
            } else {
                _config["hide_cleared_locations"] = json::value_t::null;
                _config["hide_unreachable_locations"] = json::value_t::null;
                if (_win) _win->unsetHideClearedLocations();
                if (_win) _win->unsetHideUnreachableLocations();
                if (_broadcast) _broadcast->unsetHideClearedLocations();
                if (_broadcast) _broadcast->unsetHideUnreachableLocations();
            }
        }
        else if (hotkey.id == HOTKEY_RELOAD) {
            reloadTracker();
        }
        else if (hotkey.id == HOTKEY_FORCE_RELOAD) {
            reloadTracker(true);
        }
        else if (hotkey.id == HOTKEY_TOGGLE_SPLIT_COLORS) {
            Ui::MapWidget::SplitRects = !Ui::MapWidget::SplitRects;
            _config["split_map_locations"] = Ui::MapWidget::SplitRects;
        }
        else if (hotkey.id == HOTKEY_SHOW_BROADCAST) {
            showBroadcast();
        }
        else if (hotkey.id == HOTKEY_SHOW_HELP) {
            if (SDL_OpenURL("https://github.com/black-sliver/PopTracker/blob/master/README.md") != 0) {
                Dlg::MsgBox("Error", SDL_GetError(), Dlg::Buttons::OK, Dlg::Icon::Error);
            }
        }
    }};
    _ui->addHotkey({HOTKEY_TOGGLE_VISIBILITY, SDLK_F11, KMOD_NONE});
    _ui->addHotkey({HOTKEY_TOGGLE_VISIBILITY, SDLK_h, KMOD_LCTRL});
    _ui->addHotkey({HOTKEY_TOGGLE_VISIBILITY, SDLK_h, KMOD_RCTRL});
    _ui->addHotkey({HOTKEY_FORCE_RELOAD, SDLK_F5, KMOD_LCTRL});
    _ui->addHotkey({HOTKEY_FORCE_RELOAD, SDLK_F5, KMOD_RCTRL});
    _ui->addHotkey({HOTKEY_RELOAD, SDLK_F5, KMOD_NONE});
    _ui->addHotkey({HOTKEY_FORCE_RELOAD, SDLK_r, KMOD_LCTRL|KMOD_LSHIFT});
    _ui->addHotkey({HOTKEY_FORCE_RELOAD, SDLK_r, KMOD_RCTRL|KMOD_RSHIFT});
    _ui->addHotkey({HOTKEY_RELOAD, SDLK_r, KMOD_LCTRL});
    _ui->addHotkey({HOTKEY_RELOAD, SDLK_r, KMOD_RCTRL});
    _ui->addHotkey({HOTKEY_TOGGLE_SPLIT_COLORS, SDLK_p, KMOD_LCTRL});
    _ui->addHotkey({HOTKEY_TOGGLE_SPLIT_COLORS, SDLK_p, KMOD_RCTRL});
    _ui->addHotkey({HOTKEY_SHOW_BROADCAST, SDLK_F2, KMOD_NONE});
    _ui->addHotkey({HOTKEY_SHOW_HELP, SDLK_F1, KMOD_NONE});

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
                size.width = std::max(96, std::min(8192, to_int(jSize[0],size.width)));
                size.height = std::max(96, std::min(4096, to_int(jSize[1],size.height)));
            }
            // fix invalid positioning
            _ui->makeWindowVisible(pos, size);
        }
        auto& jPack = _args.contains("pack") ? _args["pack"] : _config["pack"];
        if (jPack.type() == json::value_t::object) {
            std::string path = pathFromUTF8(to_string(jPack["path"],""));
            std::string variant = to_string(jPack["variant"],"");
            if (!scheduleLoadTracker(path,variant))
            {
                printf("Could not load pack \"%s\"... fuzzy matching\n", path.c_str());
                // try to fuzzy match by uid and version if path is missing
                std::string uid = to_string(jPack["uid"],"");
                std::string version = to_string(jPack["version"],"");
                Pack::Info info = Pack::Find(uid, version);
                if (info.path.empty()) info = Pack::Find(uid); // try without version
                if (!scheduleLoadTracker(info.path, variant)) {
                    printf("Could not load pack uid \"%s\" (\"%s\")\n", uid.c_str(), info.path.c_str());
                } else {
                    printf("found pack \"%s\"\n", info.path.c_str());
                }
            }
        }
    }
    
    // create main window
    auto icon = IMG_Load(asset("icon.png").c_str());
    _win = _ui->createWindow<Ui::DefaultTrackerWindow>("PopTracker", icon, pos, size);
    SDL_FreeSurface(icon);
	
	// SDL2 default is to disable screensaver, enable it if preferred
    if (_config.value<bool>("enable_screensaver", true))
        SDL_EnableScreenSaver();

    // set user preferences for visibility of uncleared and unreachable locations
    auto itHideCleared = _config.find("hide_cleared_locations");
    auto itHideUnreachable = _config.find("hide_unreachable_locations");
    if (itHideCleared != _config.end() && itHideCleared.value().is_boolean())
        _win->setHideClearedLocations(itHideCleared.value().get<bool>());
    if (itHideUnreachable != _config.end() && itHideUnreachable.value().is_boolean())
        _win->setHideUnreachableLocations(itHideUnreachable.value().get<bool>());

    // initialize to null == "don't care" if it does not exist, so it shows up in the config
    if (itHideCleared == _config.end()) _config["hide_cleared_locations"] = json::value_t::null;
    if (itHideUnreachable == _config.end()) _config["hide_unreachable_locations"] = json::value_t::null;

    _win->onMenuPressed += { this, [this](void*, const std::string& item, int index) {
        if (item == Ui::TrackerWindow::MENU_BROADCAST) {
            showBroadcast();
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
            reloadTracker();
        }
        if (item == Ui::TrackerWindow::MENU_TOGGLE_AUTOTRACKER)
        {
            if (!_scriptHost || !_scriptHost->getAutoTracker()) return;
            auto at = _scriptHost->getAutoTracker();
            if (!at) return;
            auto backend = at->getName(index);
            if (at->getState(index) == AutoTracker::State::Disabled) {
                if (backend == "AP") {
                    // TODO: replace inputbox by GUI overlay
                    std::string uri, slot, password;
                    if (!Dlg::InputBox("PopTracker", "Enter Archipelago host and port",
                            _atUri.empty() ? "localhost:38281" : _atUri, uri))
                        return;
                    // strip leading and trailing white space
                    strip(uri);
                    // leading slash and quote is probably a copy & paste error
                    bool badUri = uri.empty() || uri[0] == '/' || uri[0] == '\'' || uri[0] == '"';
                    if (!badUri) {
                        // check if URI is all digits, which is not fine
                        badUri = true;
                        for (char c : uri) {
                            if (!isdigit(c)) {
                                badUri = false;
                                break;
                            }
                        }
                    }
                    if (badUri) {
                        Dlg::MsgBox("PopTracker", "Please enter server as host:port!",
                                Dlg::Buttons::OK, Dlg::Icon::Error);
                        return;
                    }
                    if (!Dlg::InputBox("PopTracker", "Enter slot",
                            _atSlot.length() ? _atSlot.c_str() : "Player", slot))
                        return;
                    if (!Dlg::InputBox("PopTracker", "Enter password", "", password, true))
                        return;
                    _atUri = uri;
                    _atSlot = slot;
                    _atPassword = password;
                    if (at->enable(index, _atUri, _atSlot, _atPassword)) {
                        _autoTrackerAllDisabled = false;
                        _autoTrackerDisabled[backend] = false;
                        _config["at_uri"] = _atUri;
                        _config["at_slot"] = _atSlot;
                    }
                } else {
                    if (at->enable(index)) {
                        _autoTrackerAllDisabled = false;
                        _autoTrackerDisabled[backend] = false;
                    }
                }
            }
            else if (at->getState(index) != AutoTracker::State::Disabled) {
                at->disable(index);
                _autoTrackerDisabled[backend] = true;
                _autoTrackerAllDisabled = true;
                for (const auto& pair: _autoTrackerDisabled) {
                    if (!pair.second) {
                        _autoTrackerAllDisabled = false;
                        break;
                    }
                }
            }
        }
        if (item == Ui::TrackerWindow::MENU_CYCLE_AUTOTRACKER)
        {
            if (!_scriptHost || !_scriptHost->getAutoTracker()) return;
            auto at = _scriptHost->getAutoTracker();
            at->cycle(index); // if a back-end can have multiple modes or devices, cycle them
        }
        if (item == Ui::TrackerWindow::MENU_SAVE_STATE)
        {
            if (!_tracker) return;
            if (!_pack) return;
            std::string lastName;
            if (_exportFile.empty() || _exportUID != _pack->getUID()) {
                lastName = sanitize_filename(_pack->getGameName()) + ".json";
                if (!_exportDir.empty())
                    lastName = os_pathcat(_exportDir, lastName);
            } else {
                lastName = _exportFile;
            }

            std::string filename;
            if (!Dlg::SaveFile("Save State", lastName.c_str(), {{"JSON Files",{"*.json"}}}, filename))
                return;
#ifdef _WIN32 // windows does not add *.* filter, so we can be sure json was selected
            if (filename.length() < 5 || strcasecmp(filename.c_str() + filename.length() - 5, ".JSON") != 0)
                filename += ".json";
#endif
            auto jWindow = windowToJson(_win);
            json extra = {
                {"at_uri", _atUri},
                {"at_slot", _atSlot}
            };
            if (!jWindow.is_null())
                extra["window"] = jWindow;
            if (StateManager::saveState(_tracker, _scriptHost, _win->getHints(), extra, true, filename, true))
            {
                _exportFile = filename; // this is local encoding for fopen
                _exportUID = _pack->getUID();
                _exportDir = os_dirname(_exportFile);
            }
            else
            {
                Dlg::MsgBox("PopTracker", "Error saving state!",
                        Dlg::Buttons::OK, Dlg::Icon::Error);
            }
        }
        if (item == Ui::TrackerWindow::MENU_LOAD_STATE)
        {
            std::string lastName = _exportFile.empty() ? (_exportDir + OS_DIR_SEP) : _exportFile;
            std::string filename;
            if (!Dlg::OpenFile("Load State", lastName.c_str(), {{"JSON Files",{"*.json"}}}, filename)) return;
            if (filename != _exportFile) {
                _exportFile = filename; // this is local encoding for fopen
                _exportDir = os_dirname(_exportFile);
                _exportUID.clear();
            }
            loadState(filename);
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

    _win->onDrop += {this, [this](void *s, int x, int y, Ui::DropType type, const std::string& data) {
        if (type == Ui::DropType::FILE && data.length()>=5 && strcasecmp(data.c_str()+data.length()-5, ".json") == 0) {
            loadState(data);
        } else if ((type == Ui::DropType::FILE && data.length()>=4 && strcasecmp(data.c_str()+data.length()-4, ".zip") == 0)
                || (type == Ui::DropType::DIR)) {
            // if pack is in search path, load it
            // otherwise ask to "install" it
            std::string filename = data;
            while (filename.length()>0 && (filename[filename.length()-1] == '/' || filename[filename.length()-1] == '\\'))
                filename.pop_back();
            const char* tmp = strrchr(filename.c_str(), OS_DIR_SEP);
            std::string packname = tmp ? (std::string)(tmp+1) : filename;
            std::string path = filename;
            if (!Pack::isInSearchPath(filename)) {
                if (type == Ui::DropType::DIR && !fileExists(os_pathcat(filename, "manifest.json"))) return;
                std::string msg = "Install pack " + packname + " ?";
                if (Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::YesNo, Dlg::Icon::Question) != Dlg::Result::Yes)
                    return;
                // default to %HOME%/PopTracker/packs if that exist, otherwise
                // default to %APPDIR%/packs and fall back to %HOME%
                // TODO: go through PackManager?
                path = os_pathcat(getPackInstallDir(), packname);
                if (!copy_recursive(filename.c_str(), path.c_str())) {
                    std::string msg = "Error copying files:\n";
                    msg += strerror(errno);
                    path = os_pathcat(_homePackDir, packname);
                    mkdir_recursive(_homePackDir.c_str());
                    errno = 0;
                    if (!copy_recursive(filename.c_str(), path.c_str())) {
                        msg += "\n"; msg += strerror(errno);
                        Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::OK, Dlg::Icon::Error);
                        return;
                    }
                }
                printf("Installed to %s\n", path.c_str());
            }
            // attempt to load pack
            if (!loadTracker(path, "standard", false)) {
                _win->showOpen();
            }
        } else if (type == Ui::DropType::TEXT && strncasecmp(data.c_str(), "https://", 8)==0 && strcasecmp(data.c_str()+data.length()-4, ".zip") == 0) {
            // ask user to download and "install" pack
            if (!HTTP::is_uri(data)) {
                fprintf(stderr, "Dropped URI is not valid!\n");
                return;
            }
            const char* zipname = strrchr(data.c_str() + 8, '/');
            if (!zipname)
                return;
            zipname += 1;
            std::string msg = "Download pack from " + data + " ?";
            if (Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::YesNo, Dlg::Icon::Question) != Dlg::Result::Yes)
                return;
            // TODO: merge this with PackManager
            std::string s;
            auto requestHeaders = _httpDefaultHeaders;
            if (HTTP::Get(data, s, requestHeaders) != HTTP::OK) {
                Dlg::MsgBox("PopTracker", "Error downloading file!", Dlg::Buttons::OK, Dlg::Icon::Error);
                return;
            }
            // default to %HOME%/PopTracker/packs if that exist, otherwise
            // default to %APPDIR%/packs and fall back to %HOME%
            // TODO: use PackManager::downloadUpdate
            std::string path = os_pathcat(getPackInstallDir(), zipname);
            if (!writeFile(path, s)) {
                path = os_pathcat(_homePackDir, zipname);
                mkdir_recursive(_homePackDir.c_str());
                errno = 0;
                if (!writeFile(path, s)) {
                    std:: string msg = "Error saving file:\n";
                    msg += strerror(errno);
                    Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::OK, Dlg::Icon::Error);
                    return;
                }
            }
            printf("Download saved to %s\n", path.c_str());
            // attempt to load pack
            if (!loadTracker(path, "standard", false)) {
                _win->showOpen();
            }
        }
    }};

    // Connect pack manager to GUI
    _packManager->setConfirmationHandler([](const std::string& msg, auto cb) {
        cb(Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::YesNo, Dlg::Icon::Question) == Dlg::Result::Yes);
    });
    _packManager->onUpdateAvailable += {this, [this](void*, const std::string& uid, const std::string& version, const std::string& url, const std::string& sha256) {
        if (!_pack || _pack->getUID() != uid) return;
        if (version.empty() || url.empty() || sha256.empty()) {
            printf("Invalid update information\n");
            return;
        }
        auto info = Pack::Find(uid, version, sha256);
        if (!info.path.empty()) {
            return; // update is already installed
        }
        std::string msg = "A pack update to version " + version + " is available.\nDownload from " + url + " ?";
        if (Dlg::MsgBox("PopTracker", msg,
                Dlg::Buttons::YesNo, Dlg::Icon::Question) == Dlg::Result::Yes) {
            const auto& installDir = getPackInstallDir();
            mkdir_recursive(installDir);
            _packManager->downloadUpdate(url, installDir, uid, version, sha256);
        } else if (Dlg::MsgBox("PopTracker", "Skip version " + version + "?",
                Dlg::Buttons::YesNo, Dlg::Icon::Question) == Dlg::Result::Yes) {
            _packManager->ignoreUpdateSHA256(uid, sha256);
        } else {
            // don't bug user again until restart
            _packManager->tempIgnoreSourceVersion(uid, _pack->getVersion());
        }
    }};
    _packManager->onUpdateProgress += {this, [this](void*, const std::string& url, int received, int total) {
        // NOTE: total is 0 if size is unknown
        _win->showProgress("Downloading ...", received, total);
    }};
    _packManager->onUpdateFailed += {this, [this](void*, const std::string& url, const std::string& reason) {
        (void)url;
        // TODO: hide progress bar overlay
        _win->hideProgress();
        Dlg::MsgBox("PopTracker", "Update failed: " + reason);
    }};
    _packManager->onUpdateComplete += {this, [this](void*, const std::string& url, const std::string& file, const std::string& uid) {
        (void)url;
        _win->hideProgress();
        std::string variant = _pack ? _pack->getVariant() : "";
        if (_pack && _pack->getUID() == uid) {
            // FIXME: probably should capture which pack started the update
            std::string oldPath = _pack->getPath();
            unloadTracker();
            std::string oldDir = os_dirname(oldPath);
            std::string oldName = os_basename(oldPath);
            std::string backupDir = os_pathcat(oldDir, "old");
            mkdir_recursive(backupDir.c_str());
            std::string backupPath = os_pathcat(backupDir, oldName);
            if (pathExists(backupPath) && unlink(backupPath.c_str()) != 0) {
                fprintf(stderr, "Could not delete old backup %s: %s\n",
                        backupPath.c_str(), strerror(errno));
            } else if (rename(oldPath.c_str(), backupPath.c_str()) == 0) {
                printf("Moved %s to %s\n", oldPath.c_str(), backupPath.c_str());
            } else {
                fprintf(stderr, "Could not move %s to %s: %s\n",
                        oldPath.c_str(), backupPath.c_str(), strerror(errno));
            }
        }
        loadTracker(file, variant);
    }};

    _frameTimer = std::chrono::steady_clock::now();
    _fpsTimer = std::chrono::steady_clock::now();
    return (_win != nullptr);
}

bool PopTracker::frame()
{
    if (_asio) {
        _asio->poll();
        // when all tasks are done, poll() will stop(). Reset for next request.
        if (_asio->stopped()) _asio->restart();
    }
    if (_scriptHost)
        _scriptHost->onFrame();

    auto now = std::chrono::steady_clock::now();

#define SHOW_FPS
#ifdef SHOW_FPS
    // time since last frame
    auto td = std::chrono::duration_cast<std::chrono::milliseconds>(now - _frameTimer).count();
    if (td > _maxFrameTime) _maxFrameTime = td;
    _frameTimer = now;
    // time since last fps display
    td = std::chrono::duration_cast<std::chrono::milliseconds>(now - _fpsTimer).count();
    if (td >= 5000) {
        unsigned f = _frames*1000; f/=td;
        if (_debugFlags.count("fps") || _maxFrameTime > 1000)
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
        auto jWindow = windowToJson(_win);

        // save to config
        _config["format_version"] = 1;
        if (_pack)
            _config["pack"] = {
                {"path",pathToUTF8(_pack->getPath())},
                {"variant",_pack->getVariant()},
                {"uid",_pack->getUID()},
                {"version",_pack->getVersion()}
            };
        if (!jWindow.is_null())
            _config["window"] = jWindow;
        else
            _config.erase("window");
        _config["export_file"] = pathToUTF8(_exportFile);
        _config["export_uid"] = _exportUID;
        _config["export_dir"] = pathToUTF8(_exportDir);
        saveConfig();
    }

    // load new tracker AFTER rendering a frame
    if (res && !_newPack.empty()) {
        printf("Loading Tracker %s:%s!\n", sanitize_print(_newPack).c_str(),
                sanitize_print(_newVariant).c_str());
        if (!loadTracker(_newPack, _newVariant, _newTrackerLoadAutosave)) {
            fprintf(stderr, "Error loading pack!\n");
#ifdef DONT_IGNORE_PACK_ERRORS
            Dlg::MsgBox("PopTracker", "Error loading pack!",
                    Dlg::Buttons::OK, Dlg::Icon::Error);
            unloadTracker();
#endif
        } else {
            printf("Tracker loaded!\n");
        }
        _newPack = "";
        _newVariant = "";
    }

    // auto-save state
    if (res && _tracker && AUTOSAVE_INTERVAL>0 &&
            std::chrono::duration_cast<std::chrono::seconds>(now - _autosaveTimer).count() >= AUTOSAVE_INTERVAL)
    {
        auto jWindow = windowToJson(_win);
        json extra = {
            {"at_uri", _atUri},
            {"at_slot", _atSlot}
        };
        if (!jWindow.is_null())
            extra["window"] = jWindow;
        StateManager::saveState(_tracker, _scriptHost, _win->getHints(), extra, true);
        _autosaveTimer = std::chrono::steady_clock::now();
    }

    return res;
}

bool PopTracker::ListPacks(PackManager::confirmation_callback confirm, bool installable)
{
    json installablePacks;
    if (installable) {
        printf("Fetching data...\n");
        bool done = false;
        if (confirm) _packManager->setConfirmationHandler(confirm);
        _packManager->getAvailablePacks([&done,&installablePacks](const json& j) {
            done = true;
            installablePacks = j;
        });

        while (!done) {
            _asio->poll();
        }

        printf("\n");
    }

    printf("Search paths:\n");
    for (const auto& path: Pack::getSearchPaths()) {
        printf("%s\n", path.c_str());
    }
    printf("\n");

    printf("Installed packs:\n");
    auto installed = Pack::ListAvailable();
    if (installed.empty()) {
        printf("~ no packs installed ~\n");
    }
    for (const auto& info: installed) {
        printf("%s %s \"%s\"\n", info.uid.c_str(), info.version.c_str(), info.packName.c_str());
    }
    printf("\n");

    if (installable) {
        printf("Installable packs:\n");
        if (!installablePacks.is_object() || !installablePacks.size()) {
            printf("~ no packs in repositories ~\n");
        } else {
            for (auto& pair: installablePacks.items()) {
                printf("%s %s\n", pair.key().c_str(), pair.value()["name"].dump().c_str());
            }
        }
        printf("\n");
    }

    return true;
}

bool PopTracker::InstallPack(const std::string& uid, PackManager::confirmation_callback confirm)
{
    bool done = false;
    bool res = false;
    if (confirm) _packManager->setConfirmationHandler(confirm);
    _packManager->onUpdateProgress += {this, [](void*, const std::string& url, int received, int total) {
        int percent = total>0 ? received*100/total : 0;
        printf("%3d%% [%s/%s]        \r", percent, format_bytes(received).c_str(), format_bytes(total).c_str());
    }};
    _packManager->onUpdateComplete += {this, [&done, &res](void*, const std::string& url, const std::string& local, const std::string&) {
        printf("\nDownload complete.\n");
        printf("Pack installed as %s\n", sanitize_print(local).c_str());
        res = false;
        done = true;
    }};
    _packManager->onUpdateFailed += {this, [&done, &res](void*, const std::string& url, const std::string& msg) {
        printf("\nDownload failed: %s\n", sanitize_print(msg).c_str());
        res = false;
        done = true;
    }};
    _packManager->checkForUpdate(uid, "", "", [this](const std::string& uid, const std::string& version, const std::string& url, const std::string& sha256){
        const auto& installDir = getPackInstallDir();
        mkdir_recursive(installDir);
        _packManager->downloadUpdate(url, installDir, uid, version, sha256);
    }, [&done, &res](const std::string& uid) {
        printf("%s has no installation candidate\n", uid.c_str());
        res = false;
        done = true;
    });
    while (!done) {
        _asio->poll();
    }
    return res;
}

const std::string& PopTracker::getPackInstallDir() const
{
    return (!_isPortable && dirExists(_homePackDir)) || !isWritable(_appPackDir) ? _homePackDir : _appPackDir;
}

void PopTracker::unloadTracker()
{
    if (_tracker) {
        auto jWindow = windowToJson(_win);
        json extra = {
            {"at_uri", _atUri},
            {"at_slot", _atSlot}
        };
        if (!jWindow.is_null())
            extra["window"] = jWindow;
        StateManager::saveState(_tracker, _scriptHost, _win->getHints(), extra, true);
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
    delete _luaio;
    delete _scriptHost;
    delete _tracker;
    delete _pack;
    delete _archipelago;
    _L = nullptr;
    _luaio = nullptr;
    _tracker = nullptr;
    _scriptHost = nullptr;
    _pack = nullptr;
    _archipelago = nullptr;

    _debugFlags = _defaultDebugFlags;
}

bool PopTracker::loadTracker(const std::string& pack, const std::string& variant, bool loadAutosave)
{
    printf("Cleaning up...\n");
    unloadTracker();
    
    printf("Loading Pack...\n");
    _pack = new Pack(pack);

    bool targetLatest = !(_pack->getTargetPopTrackerVersion() > Version{});
    const Version& targetVersion = targetLatest ? VERSION : _pack->getTargetPopTrackerVersion();
    printf("Target PopTracker version: %s%s\n",
            targetVersion.to_string().c_str(),
            targetLatest ? " (undefined/latest)" : "");

    printf("Selecting Variant...\n");
    _pack->setVariant(variant);

    if (_pack->getMinPopTrackerVersion() > VERSION) {
        Dlg::MsgBox("Error", "Pack requires PopTracker " + _pack->getMinPopTrackerVersion().to_string() + " or newer. "
                    "You have " + VERSION_STRING + ".");
        fprintf(stderr, "Pack requires a newer version of PopTracker!\n");
        delete _pack;
        _pack = nullptr;
        return false;
    }
    
    printf("Creating Lua State...\n");
    _L = luaL_newstate();
    if (!_L) {
        fprintf(stderr, "Error creating Lua State!\n");
        delete _pack;
        _pack = nullptr;
        return false;
    }

    printf("Loading Lua libs...\n");
    std::initializer_list<const luaL_Reg> luaLibs = {
      {LUA_GNAME, luaopen_base},
      {LUA_TABLIBNAME, luaopen_table},
      {LUA_OSLIBNAME, luaopen_os}, // this has to be reduced in functionality
      {LUA_STRLIBNAME, luaopen_string},
      {LUA_MATHLIBNAME, luaopen_math},
      {LUA_UTF8LIBNAME, luaopen_utf8},
    };
    for (const auto& lib: luaLibs) {
        luaL_requiref(_L, lib.name, lib.func, 1);
        lua_pop(_L, 1);
    }
    // load lua debugger if enabled during compile AND run time
#ifdef WITH_LUA_DEBUG
    if (_config.value<bool>("lua_debug", false)) {
        luaL_requiref(_L, LUA_DBLIBNAME, luaopen_debug, 1);
        lua_pop(_L, 1);
    }
#endif
    // block some global function until we decide what to allow
    for (const auto& blocked: { "load", "loadfile", "loadstring" }) {
        lua_pushnil(_L);
        lua_setglobal(_L, blocked);
    }
    // implement require
    lua_pushcfunction(_L, luasandbox_require);
    lua_setglobal(_L, "require");
    // reduce os
    lua_getglobal(_L, LUA_OSLIBNAME); // get full os
    lua_createtable(_L, 0, 3); // create new os
    for (const auto& field : { "clock", "date", "difftime", "time" }) {
        lua_getfield(_L, -2, field);
        lua_setfield(_L, -2, field);
    }
    lua_setglobal(_L, LUA_OSLIBNAME); // store new os, deref old os
    _luaio = new LuaPackIO(_pack);
    LuaPackIO::Lua_Register(_L);
    LuaPackIO::File::Lua_Register(_L);
    _luaio->Lua_Push(_L);
    lua_setglobal(_L, LUA_IOLIBNAME);

    printf("Loading Tracker...\n");
    _tracker = new Tracker(_pack, _L);
    printf("Registering in Lua...\n");
    Tracker::Lua_Register(_L);
    _tracker->Lua_Push(_L);
    lua_setglobal(_L, "Tracker");
    // store pack in registry for "private" use (in require)
    lua_pushstring(_L, "Pack");
    lua_pushlightuserdata(_L, _pack);
    lua_settable(_L, LUA_REGISTRYINDEX);

    // store native debug flags set in registry
    lua_pushstring(_L, "DebugFlags");
    lua_pushlightuserdata(_L, &_debugFlags);
    lua_settable(_L, LUA_REGISTRYINDEX);

    printf("Creating Script Host...\n");
    _scriptHost = new ScriptHost(_pack, _L, _tracker);
    printf("Registering in Lua...\n");
    ScriptHost::Lua_Register(_L);
    _scriptHost->Lua_Push(_L);
    lua_setglobal(_L, "ScriptHost");
    AutoTracker* at = _scriptHost->getAutoTracker();
    if (at) {
        if (_autoTrackerAllDisabled)
            at->disable(-1); // -1 = all
        at->onError += {this,  [](void*, const std::string& msg) {
            Dlg::MsgBox("PopTracker", msg,
                    Dlg::Buttons::OK, Dlg::Icon::Error);
        }};
        auto ap = at->getAP();
        if (ap) {
            printf("Creating Archipelago Interface...\n");
            _archipelago = new Archipelago(_L, ap);
            printf("Registering in Lua...\n");
            Archipelago::Lua_Register(_L);
            _archipelago->Lua_Push(_L);
            lua_setglobal(_L, "Archipelago");
        }
        std::vector<std::string> snesAddresses;
        auto snesCfg = _config.find("usb2snes");
        if (snesCfg != _config.end() && snesCfg.value().is_string()) {
            std::string s = snesCfg.value();
            if (!s.empty()) snesAddresses.push_back(s);
        } else if (snesCfg != _config.end() && snesCfg.value().is_array()) {
            for (const auto& v: snesCfg.value())
                if (v.is_string()) {
                    std::string s = v;
                    if (!s.empty()) snesAddresses.push_back(s);
                }
        }
        at->setSnesAddresses(snesAddresses);
    }
    
    printf("Registering classes in Lua...\n");
    LuaItem::Lua_Register(_L); // TODO: move this to Tracker or ScriptHost
    JsonItem::Lua_Register(_L); // ^
    LocationSection::Lua_Register(_L); // ^
    Location::Lua_Register(_L); // ^
    
    ImageReference::Lua_Register(_L);
    _imageReference.Lua_Push(_L);
    lua_setglobal(_L, "ImageReference");
    
    // because this is a good way to detect auto-tracking support
    Lua(_L).Push("Lua 5.3");
    lua_setglobal(_L, "_VERSION");
    
    Lua(_L).Push(VERSION_STRING);
    lua_setglobal(_L, "PopVersion");
    
    // enums
    LuaEnum<AccessibilityLevel>({
        {"None", AccessibilityLevel::NONE},
        {"Partial", AccessibilityLevel::PARTIAL},
        {"Inspect", AccessibilityLevel::INSPECT},
        {"SequenceBreak", AccessibilityLevel::SEQUENCE_BREAK},
        {"Normal", AccessibilityLevel::NORMAL},
        {"Cleared", AccessibilityLevel::CLEARED},
    }).Lua_SetGlobal(_L, "AccessibilityLevel");

    printf("Hooking Lua globals...\n");
    global_wrap(_L, this);
    if (_debugFlags.empty())
        lua_pushnil(_L);
    else
        json_to_lua(_L, _debugFlags);
    lua_setglobal(_L, "DEBUG");

    if (loadAutosave) {
        // restore window size before loading any layout
        json extra = StateManager::getStateExtra(_tracker, true);
        if (extra.is_object()) {
            auto& jWindow = extra["window"];
            if (jWindow.is_object()) {
                auto& jSize = jWindow["size"];
                if (jSize.is_array() && jSize[0].is_number_integer() && jSize[1].is_number_integer()) {
                    printf("Restoring window size %dx%d\n", jSize[0].get<int>(), jSize[1].get<int>());
                    Ui::Size size = {std::max(96, jSize[0].get<int>()),
                                     std::max(96, jSize[1].get<int>())};
                    _win->setMinSize(size);
                    _win->resize(size);
                    // when making the window bigger, make sure it doesn't display garbage
                    _ui->render();
                }
            }
        }
    }

    printf("Updating UI\n");
    _win->setTracker(_tracker);
    if (auto at = _scriptHost->getAutoTracker()) {
        at->onStateChange += {this, [this](void* sender, int index, AutoTracker::State state)
        {
            AutoTracker* at = (AutoTracker*)sender;
            if (_win) _win->setAutoTrackerState(index, state, at->getName(index), at->getSubName(index));
        }};
    }
    
    // "legacy" packs don't load the items/layouts through Lua, but use fixed files
    // we support a small sub-set here
    if (_pack->hasFile("items.json"))
        _tracker->AddItems("items.json");
    if (_pack->hasFile("tracker_layout.json"))
        _tracker->AddLayouts("tracker_layout.json");
    if (_pack->hasFile("broadcast_layout.json"))
        _tracker->AddLayouts("broadcast_layout.json");
    if (_pack->hasFile("maps.json"))
        _tracker->AddMaps("maps.json");
    if (_pack->hasFile("locations.json"))
        _tracker->AddLocations("locations.json");

    // run pack's init script
    printf("Running init...\n");
    bool res = _scriptHost->LoadScript("scripts/init.lua");
    // save reset-state
    StateManager::saveState(_tracker, _scriptHost, _win->getHints(), json::value_t::null, false, "reset");
    if (loadAutosave) {
        // restore previous state
        json extra;
        if (StateManager::loadState(_tracker, _scriptHost, extra, true) && extra.is_object()) {
            if (extra["at_uri"].is_string())
                _atUri = extra["at_uri"];
            if (extra["at_slot"].is_string())
                _atSlot = extra["at_slot"];
        }
    }

    _autosaveTimer = std::chrono::steady_clock::now();

    // check for updates
    _packManager->checkForUpdate(_pack, [](const std::string&, const std::string&, const std::string&, const std::string&) {
        printf("Pack update available!\n");
    });

    return res;
}

bool PopTracker::scheduleLoadTracker(const std::string& pack, const std::string& variant, bool loadAutosave)
{
    // TODO: (load and) verify pack already?
    if (! pathExists(pack)) return false;
    _newPack = pack;
    _newVariant = variant;
    _newTrackerLoadAutosave = loadAutosave;
    return true;
}

void PopTracker::reloadTracker(bool force)
{
    if (!_tracker) return;
    if (_pack && (force || _pack->hasFilesChanged()))
        scheduleLoadTracker(_pack->getPath(), _pack->getVariant(), false);
    else {
        json extra;
        StateManager::loadState(_tracker, _scriptHost, extra, false, "reset");
        // NOTE: we ignore at_uri and at_slot for "reset"
    }
}

void PopTracker::loadState(const std::string& filename)
{
    std::string s;
    if (!readFile(filename, s)) {
        fprintf(stderr, "Error reading state file: %s\n", filename.c_str());
        Dlg::MsgBox("PopTracker", "Could not read state file!",
                Dlg::Buttons::OK, Dlg::Icon::Error);
        return;
    }
    json j;
    try {
        j = parse_jsonc(s);
    } catch (...) {
        fprintf(stderr, "Error parsing state file: %s\n", filename.c_str());
        Dlg::MsgBox("PopTracker", "Selected file is not valid json!",
                Dlg::Buttons::OK, Dlg::Icon::Error);
        return;
    }
    auto jNull = json(nullptr);
    auto& jPack = j.is_object() ? j["pack"] : jNull;
    if (!jPack.is_object() || !jPack["uid"].is_string() || !jPack["variant"].is_string()) {
        fprintf(stderr, "Json is not a state file: %s\n", filename.c_str());
        Dlg::MsgBox("PopTracker", "Selected file is not a state file!",
                Dlg::Buttons::OK, Dlg::Icon::Error);
        return;
    }
    if (_pack->getUID() != jPack["uid"] || _pack->getVariant() != jPack["variant"]) {
        auto packInfo = Pack::Find(jPack["uid"]);
        if (packInfo.uid != jPack["uid"]) {
            std::string msg = "Could not find pack: " + sanitize_print(jPack["uid"]) + "!";
            fprintf(stderr, "%s\n", msg.c_str());
            Dlg::MsgBox("PopTracker", msg,
                    Dlg::Buttons::OK, Dlg::Icon::Error);
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
            Dlg::MsgBox("PopTracker", msg,
                    Dlg::Buttons::OK, Dlg::Icon::Error);
            return;
        }
        if (!loadTracker(packInfo.path, jPack["variant"], false)) {
            fprintf(stderr, "Error loading pack!\n");
#ifdef DONT_IGNORE_PACK_ERRORS
            Dlg::MsgBox("PopTracker", "Error loading pack!",
                    Dlg::Buttons::OK, Dlg::Icon::Error);
            unloadTracker();
#endif
        }
    }
    json extra;
    if (!StateManager::loadState(_tracker, _scriptHost, extra, true, filename, true)) {
        Dlg::MsgBox("PopTracker", "Error loading state!",
                Dlg::Buttons::OK, Dlg::Icon::Error);
    }
    else if (extra.is_object()) {
        if (extra["at_uri"].is_string())
            _atUri = extra["at_uri"];
        if (extra["at_slot"].is_string())
            _atSlot = extra["at_slot"];
    }
    if (_pack) _exportUID = _pack->getUID();
}

void PopTracker::showBroadcast()
{
#ifndef __EMSCRIPTEN__ // no multi-window support (yet)
    if (!_tracker)
        return;

    if (_broadcast) {
        _broadcast->Raise();
    } else if (_tracker->hasLayout("tracker_broadcast")) {
        // create window
        auto icon = IMG_Load(asset("icon.png").c_str());
        Ui::Position pos = _win->getPosition() + Ui::Size{0,32};
        _broadcast = _ui->createWindow<Ui::BroadcastWindow>("Broadcast", icon, pos);
        SDL_FreeSurface(icon);
        // set user preferences for visibility of uncleared and unreachable locations
        auto itHideCleared = _config.find("hide_cleared_locations");
        auto itHideUnreachable = _config.find("hide_unreachable_locations");
        if (itHideCleared != _config.end() && itHideCleared.value().is_boolean())
            _broadcast->setHideClearedLocations(itHideCleared.value().get<bool>());
        if (itHideUnreachable != _config.end() && itHideUnreachable.value().is_boolean())
            _broadcast->setHideUnreachableLocations(itHideUnreachable.value().get<bool>());
        // set up window
        _broadcast->setTracker(_tracker);
        pos += Ui::Size{_win->getWidth()/2, _win->getHeight()/2 - 32};
        _broadcast->setCenterPosition(pos); // this will reposition the window after rendering
    }
#endif
}

void PopTracker::updateAvailable(const std::string& version, const std::string& url, const std::list<std::string> assets)
{
    std::string ignoreData;
    std::string ignoreFilename = getConfigPath(APPNAME, "ignored-versions.json", _isPortable);
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
    if (Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::YesNo, Dlg::Icon::Question) == Dlg::Result::Yes)
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
            Dlg::MsgBox("PopTracker", msg,
                    Dlg::Buttons::OK, Dlg::Icon::Error);
            exit(0);
        }
#endif
    }
    else {
        msg = "Skip version " + version + "?";
        if (Dlg::MsgBox("PopTracker", msg, Dlg::Buttons::YesNo, Dlg::Icon::Question) == Dlg::Result::Yes)
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
    return v > VERSION;
}

bool PopTracker::saveConfig()
{
    bool res = true;
    if (_oldConfig != _config) {
        std::string configDir = getConfigPath(APPNAME, "", _isPortable);
        mkdir_recursive(configDir.c_str());
        res = writeFile(os_pathcat(configDir, std::string(APPNAME)+".json"), _config.dump(4)+"\n");
        if (res) _oldConfig = _config;
    }
    return res;
}
