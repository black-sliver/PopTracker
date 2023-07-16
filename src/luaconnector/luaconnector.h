#ifndef _LUACONNECTOR_H
#define _LUACONNECTOR_H

#include "../core/autotrackprovider.h"
#include <chrono>
#include "../core/tsbuffer.h"

namespace LuaConnector {

namespace Net {
class Server;
}

typedef std::pair<uint32_t, unsigned int> Watch;
typedef std::vector<Watch> Watchlist;

class LuaConnector : public IAutotrackProvider {
public:
    LuaConnector(const std::string& appname);
    ~LuaConnector();

    const std::string& getName() override;

    bool start() override;
    bool stop() override;

    bool update() override;

    bool isReady() override;
    bool isConnected() override;

    void clearCache() override;

    void addWatch(uint32_t address, unsigned int length) override;
    void removeWatch(uint32_t address, unsigned int length) override;
    void setWatchUpdateInterval(size_t interval) override;

    void setMapping(const std::set<std::string>& flags) override;
    uint32_t mapAddress(uint32_t address) override;

    bool readFromCache(uint32_t address, unsigned int length, void* out) override;
    uint8_t readUInt8FromCache(uint32_t address, uint32_t offset = 0) override;
    uint16_t readUInt16FromCache(uint32_t address, uint32_t offset = 0) override;
    uint32_t readUInt32FromCache(uint32_t address, uint32_t offset = 0) override;

    uint8_t readU8Live(uint32_t address, uint32_t offset = 0) override;
    uint16_t readU16Live(uint32_t address, uint32_t offset = 0) override;

private:

    void recalculate_watches();

    Net::Server* _server = nullptr;

    std::string _appname;
    const std::string _name = "Lua";

    tsbuffer<uint8_t> _data;

    std::chrono::time_point<std::chrono::system_clock> _lastWatchCheck;
    Watchlist _watches;
    Watchlist _combinedWatches;
    bool _recalculateWatches = true;

    uint64_t _watchRefreshMilliseconds = 1500;
};

}

#endif // _LUACONNECTOR_H
