#include "gameinfo.h"
#include <map>
#include <string>
#include <algorithm>


static const GameInfo _blank;

const std::map<std::string, GameInfo> GameInfo::_games = {
    { "super metroid + lttp randomizer", {
        "Super Metroid + LTTP Randomizer",
        {},
        {"exhirom"},
    }},
    { "super metroid randomizer", {
        "Super Metroid Randomizer",
        {},
        {"lorom"},
    }},
    { "a link to the past randomizer", {
        "A Link to the Past Randomizer",
        {"alttpr"},
        {"lorom"},
    }},
    { "evermizer", {
        "Evermizer",
        {"secret of evermore randomizer"},
        {"hirom"},
    }},
    { "secret of evermore", {
        "Secret of Evermore",
        {},
        {"hirom"},
    }},
    { "secret of mana randomizer", {
        "Secret of Mana Randomizer",
        {"somr"},
        {"hirom"},
    }},
    { "secret of mana", {
        "Secret of Mana",
        {"seiken densetsu 2"},
        {"hirom"},
    }},
    { "illusion of gaia randomizer", {
        "Illusion of Gaia Randomizer",
        {"iogr"},
        {"hirom"},
    }},
    { "soul blazer randomizer", {
        "Soul Blazer Randomizer",
        {},
        {"lorom"},
    }},
};


const GameInfo& GameInfo::Find(const std::string& name)
{
    std::string s = name;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    
    // try exact lower-case match
    const auto it = _games.find(s);
    if (it != _games.end()) return it->second;

    // search alt names
    for (const auto& pair: _games)
        if (pair.second.alt_names.find(s) != pair.second.alt_names.end())
            return pair.second;
    
    return _blank;
}