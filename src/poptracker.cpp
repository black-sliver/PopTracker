#include "poptracker.h"
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
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
#ifdef _WIN32
#include <windows.h>
#endif
using nlohmann::json;
using Ui::Dlg;


PopTracker::PopTracker(int argc, char** argv, bool cli)
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
    } else if (!cli) {
        // enable logging
        printf("Logging to %s\n", logFilename.c_str());
        if (Log::RedirectStdOut(logFilename)) {
            printf("%s %s\n", APPNAME, VERSION_STRING);
        }

#ifndef WITHOUT_UPDATE_CHECK
        if (_config.find("check_for_updates") == _config.end()) {
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

    if (_config["export_file"].is_string() && _config["export_uid"].is_string()) {
        _exportFile = _config["export_file"];
        _exportUID = _config["export_uid"];
    }
    if (_config["export_dir"].is_string())
        _exportDir = _config["export_dir"];

    if (_config["at_uri"].is_string())
        _atUri = _config["at_uri"];
    if (_config["at_slot"].is_string())
        _atSlot = _config["at_slot"];

    if (_config["usb2snes"].is_null())
        _config["usb2snes"] = "";

    saveConfig();

    _ui = nullptr; // UI init moved to start()

    Pack::addSearchPath("packs"); // current directory

    std::string cwdPath = getCwd();
    std::string documentsPath = getDocumentsPath();
    std::string homePath = getHomePath();
    std::string appPath = getAppPath();

    _homePackDir = os_pathcat(homePath, "PopTracker", "packs");
    _appPackDir = os_pathcat(appPath, "packs");

    if (!homePath.empty() && homePath != "." && homePath != cwdPath) {
        Pack::addSearchPath(_homePackDir); // default user packs
        Assets::addSearchPath(os_pathcat(homePath,"PopTracker","assets")); // default user overrides
    }
    if (!documentsPath.empty() && documentsPath != "." && documentsPath != cwdPath) {
        Pack::addSearchPath(os_pathcat(documentsPath,"PopTracker","packs")); // alternative user packs
        if (_config.value<bool>("add_emo_packs", false)) {
            Pack::addSearchPath(os_pathcat(documentsPath,"EmoTracker","packs")); // "old" packs
        }
    }
    if (!appPath.empty() && appPath != "." && appPath != cwdPath) {
        Pack::addSearchPath(_appPackDir); // system packs
        Assets::addSearchPath(os_pathcat(appPath,"assets")); // system assets
    }

    _asio = new asio::io_service();
    HTTP::certfile = asset("cacert.pem"); // https://curl.se/docs/caextract.html

    _packManager = new PackManager(_asio, getConfigPath(APPNAME), _httpDefaultHeaders);
    // TODO: move repositories to config?
    _packManager->addRepository("https://raw.githubusercontent.com/black-sliver/PopTracker/packlist/community-packs.json");
    // NOTE: signals are connected later to allow gui and non-gui interaction

    StateManager::setDir(getConfigPath(APPNAME, "saves"));
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
                if (info.path.empty()) info = Pack::Find(uid); // try without version
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
    
    _win->onMenuPressed += { this, [this](void*, const std::string& item, int index) {
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
            if (!_tracker) return;
            if (_pack && _pack->hasFilesChanged())
                scheduleLoadTracker(_pack->getPath(), _pack->getVariant(), false);
            else
                StateManager::loadState(_tracker, _scriptHost, false, "reset");
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
                    if (!Dlg::InputBox("PopTracker", "Enter archipelago server:port", _atUri.empty()?"localhost":_atUri, uri)) return;
                    if (!Dlg::InputBox("PopTracker", "Enter slot", (_atUri==uri)?_atSlot.c_str():"Player", slot)) return;
                    if (!Dlg::InputBox("PopTracker", "Enter password", "", password, true)) return;
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

            std::string filename;
            if (!Dlg::SaveFile("Save State", lastName.c_str(), {{"JSON Files",{"*.json"}}}, filename)) return;
            if (StateManager::saveState(_tracker, _scriptHost,
                    _win->getHints(), true, filename, true))
            {
                _exportFile = filename;
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
                _exportFile = filename;
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
            const char* zipname = strrchr(data.c_str() + 8, '/');
            if (!zipname) return;
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
            if (rename(oldPath.c_str(), backupPath.c_str()) == 0) {
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
    if (_scriptHost) _scriptHost->autoTrack();

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
        StateManager::saveState(_tracker, _scriptHost, _win->getHints(), true);
        _autosaveTimer = std::chrono::steady_clock::now();
    }

    return res;
}

bool PopTracker::ListPacks(PackManager::confirmation_callback confirm)
{
    printf("Fetching data...\n");
    bool done = false;
    json installable;
    if (confirm) _packManager->setConfirmationHandler(confirm);
    _packManager->getAvailablePacks([&done,&installable](const json& j) {
        done = true;
        installable = j;
    });

    while (!done) {
        _asio->poll();
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
    printf("Installable packs:\n");
    if (!installable.is_object() || !installable.size()) {
        printf("~ no packs in repositories ~\n");
    } else {
        for (auto& pair: installable.items()) {
            printf("%s %s\n", pair.key().c_str(), pair.value()["name"].dump().c_str());
        }
    }

    printf("\n");
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
    return dirExists(_homePackDir) || !isWritable(_appPackDir) ? _homePackDir : _appPackDir;
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
    delete _archipelago;
    _L = nullptr;
    _tracker = nullptr;
    _scriptHost = nullptr;
    _pack = nullptr;
    _archipelago = nullptr;
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
        at->onStateChange += {this, [this](void* sender, int index, AutoTracker::State state)
        {
            AutoTracker* at = (AutoTracker*)sender;
            if (_win) _win->setAutoTrackerState(index, state, at->getName(index));
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
    printf("Running scripts/init.lua\n");
    bool res = _scriptHost->LoadScript("scripts/init.lua");
    // save reset-state
    StateManager::saveState(_tracker, _scriptHost, _win->getHints(), false, "reset");
    if (loadAutosave) {
        // restore previous state
        StateManager::loadState(_tracker, _scriptHost, true);
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
    if (!StateManager::loadState(_tracker, _scriptHost, true, filename, true)) {
        Dlg::MsgBox("PopTracker", "Error loading state!",
                Dlg::Buttons::OK, Dlg::Icon::Error);
    }
    if (_pack) _exportUID = _pack->getUID();
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
