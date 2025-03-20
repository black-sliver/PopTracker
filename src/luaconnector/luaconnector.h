#pragma once

#include "../core/autotrackprovider.h"
#include <stdint.h>
#include <string>
#include "../core/tsbuffer.h"
#include <chrono>

#if (defined __cplusplus && __cplusplus >= 201703L) || (defined __has_include && __has_include(<optional>))
#include <optional>
#else
#include <experimental/optional>
#endif

namespace LuaConnector {

namespace Net {
class Server;
}

typedef std::pair<uint32_t, unsigned int> Watch;
typedef std::vector<Watch> Watchlist;

class LuaConnector : public IAutotrackProvider {
public:
#if defined __cpp_lib_optional
    template<class T>
    using optional = std::optional<T>;
#else
    template<class T>
    using optional = std::experimental::optional<T>;
#endif

    LuaConnector(const std::string& appname, optional<std::string> defaultDomain);
    ~LuaConnector() override;

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

    bool readFromCache(void* out, uint32_t address, unsigned int length) override;
    bool readUInt8FromCache(uint8_t& out, uint32_t address, uint32_t offset = 0) override;
    bool readUInt16FromCache(uint16_t& out, uint32_t address, uint32_t offset = 0) override;
    bool readUInt32FromCache(uint32_t& out, uint32_t address, uint32_t offset = 0) override;

    //uint8_t readU8Live(uint32_t address, uint32_t offset = 0) override;
    //uint16_t readU16Live(uint32_t address, uint32_t offset = 0) override;

private:

    std::vector<Watch> combine_watches(std::vector<Watch> src, bool& bChanged);
    void recalculate_watches();

    Net::Server* _server = nullptr;

    std::string _appname;
    const std::string _name = "Lua";

    tsbuffer<uint8_t> _data;

    std::chrono::time_point<std::chrono::system_clock> _lastWatchCheck;
    Watchlist _watches;
    Watchlist _combinedWatches;
    bool _recalculateWatches = true;
    optional<std::string> _defaultDomain;

    uint64_t _watchRefreshMilliseconds = 1500;
};

} // namespace LuaConnector
