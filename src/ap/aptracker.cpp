#include "aptracker.h"
#include <map>
#include <string>


const std::map<std::string, std::string> APTracker::_errorMessages = {
    { "InvalidSlot", "Invalid Slot; please verify that you have connected to the correct world." },
    { "InvalidGame", "Invalid Game; please verify that you connected with the right game to the correct world." },
    { "SlotAlreadyTaken", "Player slot already in use for that team" },
    { "IncompatibleVersion", "Server reported your client version as incompatible" },
    { "InvalidPassword", "Wrong password" },
    { "InvalidItemsHandling", "The item handling flags requested by the client are not supported" }
};