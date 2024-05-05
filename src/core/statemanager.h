#ifndef _CORE_STATEMANAGER_H
#define _CORE_STATEMANAGER_H

#include <string>
#include <map>
#include "tracker.h"
#include "scripthost.h"
#include <nlohmann/json.hpp>

class StateManager {
public:
    static bool saveState(Tracker* tracker, ScriptHost* scripthost,
            std::list< std::pair<std::string,std::string> > uiHints={},
            const json& extra = json::value_t::null,
            bool file=false, const std::string& name="autosave",
            bool external=false);
    static bool loadState(Tracker* tracker, ScriptHost* scripthost, json& extra_out,
            bool file=false, const std::string& name="autosave",
            bool external=false);
    static json getStateExtra(Tracker* tracker,
            bool file=false, const std::string& name="autosave",
            bool external=false);
    static void setDir(const std::string& dir);
    
private:
    struct StateID {
        std::string pack_uid;
        std::string pack_version;
        std::string pack_variant;
        std::string state_name;
        
        bool operator<(const StateID& other) const {
            if (pack_uid<other.pack_uid) return true;
            if (pack_uid>other.pack_uid) return false;
            if (pack_version<other.pack_version) return true;
            if (pack_version>other.pack_version) return false;
            if (pack_variant<other.pack_variant) return true;
            if (pack_variant>other.pack_variant) return false;
            if (state_name<other.state_name) return true;
            if (state_name>other.state_name) return false;
            return false;
        }
    };
    
    static std::map<StateID, nlohmann::json> _states;
    static std::string _dir;
};

#endif /* _CORE_STATEMANAGER_H */

