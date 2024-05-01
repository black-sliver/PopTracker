#ifndef _POPTRACKER_H
#define _POPTRACKER_H

#include "version.h"
#include "app.h"
#include "uilib/ui.h"
#include "ui/defaulttrackerwindow.h"
#include "ui/broadcastwindow.h"
#include "ui/settingswindow.h"
#include <luaglue/luapp.h>
#include "core/pack.h"
#include "core/tracker.h"
#include "core/scripthost.h"
#include "core/imagereference.h"
#include "core/version.h"
#include "ap/archipelago.h"
#include "packmanager/packmanager.h"
#include "luasandbox/luapackio.h"
#include <chrono>
#include <nlohmann/json.hpp>

class PopTracker final : public App {
private:
    Ui::Ui *_ui = nullptr;
    Ui::DefaultTrackerWindow *_win = nullptr;
    Ui::BroadcastWindow *_broadcast = nullptr;
    Ui::SettingsWindow *_settings = nullptr;
    nlohmann::json _args;
    nlohmann::json _config;
    nlohmann::json _oldConfig;
    nlohmann::json _colors;
    bool _isPortable = false;
    
    lua_State* _L = nullptr;
    LuaPackIO *_luaio = nullptr;
    Tracker *_tracker = nullptr;
    ScriptHost *_scriptHost = nullptr;
    Archipelago *_archipelago = nullptr;
    Pack *_pack = nullptr;
    ImageReference _imageReference;
    bool _autoTrackerAllDisabled = false;
    std::map<std::string, bool> _autoTrackerDisabled;
    asio::io_service *_asio = nullptr;
    std::list<std::string> _httpDefaultHeaders;
    PackManager *_packManager = nullptr;
    std::string _exportFile;
    std::string _exportUID;
    std::string _exportDir;
    std::string _homePackDir;
    std::string _appPackDir;
    std::set<std::string> _debugFlags;
    std::set<std::string> _defaultDebugFlags;

    unsigned _frames = 0;
    unsigned _maxFrameTime = 0;
    std::chrono::steady_clock::time_point _fpsTimer;
    std::chrono::steady_clock::time_point _frameTimer;
    
    std::string _newPack;
    std::string _newVariant;
    bool _newTrackerLoadAutosave = false;
    std::chrono::steady_clock::time_point _autosaveTimer;

    std::string _atUri, _atSlot, _atPassword;

    bool loadTracker(const std::string& pack, const std::string& variant, bool loadAutosave=true);
    bool scheduleLoadTracker(const std::string& pack, const std::string& variant, bool loadAutosave=true);
    void unloadTracker();
    void reloadTracker(bool force=false);
    void loadState(const std::string& filename);
    void showBroadcast();
    
    void updateAvailable(const std::string& version, const std::string& url, const std::list<std::string> assets);
    static bool isNewer(const Version& v);

    const std::string& getPackInstallDir() const;

    bool saveConfig();

    static int global_index(lua_State* L);
    static int global_newindex(lua_State* L);
    static void global_wrap(lua_State* L, PopTracker* self);

public:
    PopTracker(int argc, char** argv, bool cli=false, const json& args=nullptr);
    virtual ~PopTracker();

    bool ListPacks(PackManager::confirmation_callback confirm = nullptr, bool installable = true);
    bool InstallPack(const std::string& uid, PackManager::confirmation_callback confirm = nullptr);

    static constexpr std::initializer_list<const char*> ALL_DEBUG_FLAGS = {"errors", "fps"};

    static constexpr const char APPNAME[] = "PopTracker";
    static constexpr const char VERSION_STRING[] = APP_VERSION_STRING;
    static const Version VERSION;
    static constexpr int AUTOSAVE_INTERVAL = 60; // 1 minute

protected:
    virtual bool start();
    virtual bool frame();
};

#endif // _POPTRACKER_H
