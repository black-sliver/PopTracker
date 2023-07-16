#include "luaconnector.h"
#include "common.h"
#include "../core/util.h"
#include "server.h"
#include <bitset>

namespace LuaConnector {
LuaConnector::LuaConnector(const std::string& name)
{
    printf("LuaConnector(%s)\n", sanitize_print(name).c_str());
    _appname = name;
}

LuaConnector::~LuaConnector()
{
    printf("~LuaConnector()\n");

    stop();
}

const std::string& LuaConnector::getName()
{
    return _name;
}

bool LuaConnector::start()
{
    try {
        if (!_server) {
            _server = new Net::Server(_data);
            _server->Start();
        }
    }
    catch (const std::exception& e) {
        printf("LuaConnector Exception: %s\n", e.what());
        return false;
    }

    return true;
}

bool LuaConnector::stop()
{
    try {
        if (_server) {
            delete _server;
            _server = nullptr;
        }
    }
    catch (const std::exception& e) {
        printf("LuaConnector Exception: %s\n", e.what());
        return false;
    }

    return false;
}

bool LuaConnector::update()
{
    bool data_changed = false;

    if (_server) {
        if (_recalculateWatches) {
            recalculate_watches();
            _recalculateWatches = false;
        }

        // check watches
        if (_server->ClientIsConnected()
            && _lastWatchCheck + std::chrono::milliseconds(_watchRefreshMilliseconds) < std::chrono::system_clock::now()) {
            //printf("Checking watches\n");
            _lastWatchCheck = std::chrono::system_clock::now();
            for (auto& w : _combinedWatches) // use _combinedWatches in prod
            {
                data_changed |= _server->ReadBlockBuffered(w.first, w.second);

                if (!_server->ClientIsConnected())
                    break;
            }
        }

        data_changed |= _server->Update();

        //if( data_changed )
        //{
        //	std::bitset<8> t = _data[mapaddr(0x8011AB07)];
        //	printf("Mido's house: %s\n", t.to_string().c_str());
        //}
    }

    return data_changed;
}

bool LuaConnector::isReady()
{
    return _server && _server->IsListening();
}

bool LuaConnector::isConnected()
{
    return _server && _server->ClientIsConnected();
}

void LuaConnector::clearCache()
{
    _data.clear();
}

void LuaConnector::addWatch(uint32_t address, unsigned int length)
{
    //printf("addWatch\n");
    address = mapAddress(address);

    for (auto& w : _watches) {
        if (w.first == address && w.second == length)
            return;
    }

    _watches.push_back(std::make_pair(address, length));
    _recalculateWatches = true;
}

void LuaConnector::removeWatch(uint32_t address, unsigned int length)
{
    address = mapAddress(address);
    _watches.erase(std::remove(_watches.begin(), _watches.end(), std::make_pair(address, length)));
    _recalculateWatches = true;
}

void LuaConnector::setWatchUpdateInterval(size_t interval)
{
    _watchRefreshMilliseconds = interval;
}

void LuaConnector::setMapping(const std::set<std::string>& flags)
{
}

uint32_t LuaConnector::mapAddress(uint32_t address)
{
    if (address >= 0x80000000)
        return address - 0x80000000;
    else
        return address;
}

bool LuaConnector::readFromCache(uint32_t address, unsigned int length, void* out)
{
    address = mapAddress(address);
    _data.read(address, length, out);
    return true;
}

uint8_t LuaConnector::readUInt8FromCache(uint32_t address, uint32_t offset)
{
    address = mapAddress(address);
    return _data[address];
}

uint16_t LuaConnector::readUInt16FromCache(uint32_t address, uint32_t offset)
{
    address = mapAddress(address);
    return _data.readInt<uint16_t>(address);
}

uint32_t LuaConnector::readUInt32FromCache(uint32_t address, uint32_t offset)
{
    address = mapAddress(address);
    return _data.readInt<uint32_t>(address);
}

// returns watches combined
std::vector<Watch> combine_watches(std::vector<Watch> src, bool& bChanged)
{
    bChanged = false;
    uint32_t match_distance = 0xFF;

    Watchlist out;

    // for each watch
    for (auto& w : src) {
        bool matched = false;

        uint32_t w_begin = w.first;
        uint32_t w_end = w.first + w.second;

        // find a combined watch that's close to it
        for (auto& c : out) {
            uint32_t c_begin = c.first;
            uint32_t c_end = c.first + c.second;

            // we are near the end of combined watch
            uint32_t gap = w_begin - c_end;
            if (gap < match_distance) {
                // extend end of combined watch to encompass us
                c.second += gap + w.second;

                matched = true;
                bChanged = true;
            }

            // we are near the start of combined watch
            gap = c_begin - w_end;
            if (gap < match_distance) {
                // extend beginning of combined watch to encompass us
                c.first -= gap - w.second;
                c.second += gap + w.second;

                matched = true;
                bChanged = true;
            }
        }

        // did not match
        if (!matched) {
            out.push_back(w);
        }
    }

    return out;
}

uint8_t LuaConnector::readU8Live(uint32_t address, uint32_t offset)
{
    address = mapAddress(address);
    if (_server && _server->ClientIsConnected()) {
        return _server->ReadU8Live(address);
    }
    return 0;
}

uint16_t LuaConnector::readU16Live(uint32_t address, uint32_t offset)
{
    address = mapAddress(address);
    if (_server && _server->ClientIsConnected()) {
        uint16_t result = _server->ReadU16Live(address);

        // for backwards compatibility with EmoTracker,
        // we reverse the byte order
        uint16_t swapped = ((result & 0xff) << 8) | ((result & 0xff00) >> 8);

        return swapped;
    }
    return 0;
}

void LuaConnector::recalculate_watches()
{
    printf("Recalculating watches\n");

    _combinedWatches.clear();
    Watchlist w = _watches;

    bool changed = true;
    while (changed)
        w = combine_watches(w, changed);

    _combinedWatches = w;
}

}

