#include "statemanager.h"
#include "jsonutil.h"
#include "fileutil.h"
#include "util.h"
#include <nlohmann/json.hpp>
using nlohmann::json;

std::map<StateManager::StateID, json> StateManager::_states;
fs::path StateManager::_dir;

void StateManager::setDir(const fs::path& dir)
{
    _dir = dir;
}

static void replayHints(Tracker* tracker, json& state)
{    
    // replay stored ui hints
    tracker->onUiHint.emit(tracker, "reset","reset");
    if (state["ui_hints"].type() == json::value_t::array) {
        for (auto& hint: state["ui_hints"]) {
            if (hint.type() == json::value_t::array && hint.size()>=2) {
                tracker->onUiHint.emit(tracker, to_string(hint[0],""), to_string(hint[1],""));
            }
        }
    }
}

bool StateManager::saveState(Tracker* tracker, ScriptHost*,
        std::list< std::pair<std::string,std::string> > uiHints, const json& extra,
        bool tofile, const std::string& name, bool external)
{
    if (!tracker) return false;
    auto pack = tracker->getPack();
    if (!pack) return false;
    
    auto state = tracker->saveState();
    
    auto jUiHints = json::array();
    for (auto& hint: uiHints) {
        jUiHints.push_back( {hint.first,hint.second} );
    }
    state["ui_hints"] = jUiHints;
    state["pack"] = {
        { "path", pack->getPath().u8string() },
        { "uid", pack->getUID() },
        { "variant", pack->getVariant() },
        { "version", pack->getVersion() },
    };
    state["extra"] = extra;

    if (!tofile) {
        printf("Saving state \"%s\" to RAM...\n", name.c_str());
        _states[{ pack->getUID(), pack->getVersion(),
                  pack->getVariant(), name }] = state;
        return true;
    } else {
        fs::path filename;
        if (external) {
            filename = name;
        } else {
            auto dirname = _dir /
                    sanitize_dir(pack->getUID()) /
                    sanitize_dir(pack->getVersion()) /
                    sanitize_dir(pack->getVariant());
            fs::create_directories(dirname);
            filename = dirname / (name + ".json");
        }
        printf("Saving state \"%s\" to file %s...\n",
                external ? "export" : name.c_str(), sanitize_print(filename).c_str());
        try {
            std::string new_state = state.dump();
            std::string old_state;
            if (!readFile(filename, old_state) || old_state != new_state)
                return writeFile(filename, new_state);
        } catch (std::exception& ex) {
            fprintf(stderr, "error saving: %s\n", ex.what());
            return false;
        }
        return true;
    }    
}

bool StateManager::loadState(Tracker* tracker, ScriptHost* scripthost, json& extra_out,
        bool fromfile, const std::string& name, bool external)
{
    if (!tracker) return false;
    auto pack = tracker->getPack();
    if (!pack) return false;
    
    bool res = false;
    if (!fromfile) {
        auto it = _states.find({ pack->getUID(), pack->getVersion(), pack->getVariant(), name });
        printf("Loading state \"%s\" from RAM...", name.c_str());
        if (it == _states.end()) {
            printf(" missing\n");
            return false;
        }
        printf("\n");
        res = tracker->loadState(it->second);
        replayHints(tracker, it->second);
        extra_out = it->second["extra"];
    } else {
        std::string s;
        fs::path filename = external ? fs::u8path(name) : (_dir /
                sanitize_dir(pack->getUID()) /
                sanitize_dir(pack->getVersion()) /
                sanitize_dir(pack->getVariant()) / (name + ".json"));
        printf("Loading state \"%s\" from file %s...",
                external ? "import" : name.c_str(), sanitize_print(filename).c_str());
        if (!readFile(filename, s)) {
            printf(" missing\n");
            return false;
        }
        printf("\n");
        auto j = parse_jsonc(s);
        res = tracker->loadState(j);
        replayHints(tracker, j);
        extra_out = j["extra"];
    }
    if (scripthost) {
        scripthost->resetWatches();
        if (scripthost->getAutoTracker())
            scripthost->getAutoTracker()->sync();
    }
    printf("%s\n", res ? "ok" : "error");
    return res;
}

json StateManager::getStateExtra(Tracker* tracker,
        bool fromfile, const std::string& name, bool external)
{
    if (!tracker)
        return false;
    auto pack = tracker->getPack();
    if (!pack)
        return false;

    if (!fromfile) {
        auto it = _states.find({ pack->getUID(), pack->getVersion(), pack->getVariant(), name });
        if (it != _states.end()) {
            return it->second["extra"];
        }
    } else {
        std::string s;
        fs::path filename = external ? fs::path(name) : _dir /
                sanitize_dir(pack->getUID()) /
                sanitize_dir(pack->getVersion()) /
                sanitize_dir(pack->getVariant()) / (name+".json");
        if (readFile(filename, s)) {
            auto j = parse_jsonc(s);
            if (j.is_object())
                return j["extra"];
        }
    }
    return nullptr;
}
