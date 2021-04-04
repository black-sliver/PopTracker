#include "statemanager.h"
#include "jsonutil.h"
#include "fileutil.h"
#include <json/json.hpp>
using nlohmann::json;

std::map<StateManager::StateID, json> StateManager::_states;
std::string StateManager::_dir;

static std::string sanitize(std::string s)
{
    if (s.empty()) return "_";
    if ((size_t)std::count(s.begin(), s.end(), '.') == s.length()) return "_";
    std::replace(s.begin(), s.end(), '/', '_');
    std::replace(s.begin(), s.end(), '\\', '_');
    std::replace(s.begin(), s.end(), ':', '_');
    return s;
}

void StateManager::setDir(const std::string& dir)
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
        std::list< std::pair<std::string,std::string> > uiHints,
        bool tofile, const std::string& name)
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
    
    if (!tofile) {
        printf("Saving state \"%s\" to RAM...\n", name.c_str());
        _states[{ pack->getUID(), pack->getVersion(),
                  pack->getVariant(), name }] = state;
        return true;
    } else {
        auto dirname = os_pathcat(_dir, sanitize(pack->getUID()), sanitize(pack->getVersion()), sanitize(pack->getVariant()));
        mkdir_recursive(dirname.c_str());
        auto filename = os_pathcat(dirname, name+".json");
        printf("Saving state \"%s\" to file...\n", name.c_str());
        return writeFile(filename, state.dump());
    }    
}
bool StateManager::loadState(Tracker* tracker, ScriptHost* scripthost, bool fromfile, const std::string& name)
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
    } else {
        std::string s;
        std::string filename = os_pathcat(_dir, sanitize(pack->getUID()), sanitize(pack->getVersion()), sanitize(pack->getVariant()), name+".json");
        printf("Loading state \"%s\" from file %s...", name.c_str(), filename.c_str());
        if (!readFile(filename, s)) {
            printf(" missing\n");
            return false;
        }
        printf("\n");
        auto j = parse_jsonc(s);
        res = tracker->loadState(j);
        replayHints(tracker, j);
    }
    if (scripthost) scripthost->resetWatches();
    printf("%s\n", res ? "ok" : "error");
    return res;
}