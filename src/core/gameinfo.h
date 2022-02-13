#ifndef _CORE_GAMEINFO_H
#define _CORE_GAMEINFO_H


#include <string>
#include <map>
#include <set>


struct GameInfo {
    std::string name;
    std::set<std::string> alt_names;
    std::set<std::string> flags;
    
    static const GameInfo& Find(const std::string& name);

private:
    static const std::map<std::string, GameInfo> _games;
};


#endif // _CORE_GAMEINFO_H
