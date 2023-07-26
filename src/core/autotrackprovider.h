#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <set>

class IAutotrackProvider {
public:
    virtual ~IAutotrackProvider() = default;

    virtual const std::string& getName() = 0;

    virtual bool start() = 0;
    virtual bool stop() = 0;

    // Returns true if cache was changed
    virtual bool update() = 0;

    virtual bool isReady() = 0;
    virtual bool isConnected() = 0;

    virtual void clearCache() = 0;

    virtual void addWatch(uint32_t address, unsigned int length) = 0;
    virtual void removeWatch(uint32_t address, unsigned int length) = 0;
    virtual void setWatchUpdateInterval(size_t interval) = 0;

    virtual void setMapping(const std::set<std::string>& flags) = 0;
    virtual uint32_t mapAddress(uint32_t address) = 0;

    virtual bool readFromCache(uint32_t address, unsigned int length, void* out) = 0;
    virtual uint8_t readUInt8FromCache(uint32_t address, uint32_t offset = 0) { return 0; }
    virtual uint16_t readUInt16FromCache(uint32_t address, uint32_t offset = 0) { return 0; }
    virtual uint32_t readUInt32FromCache(uint32_t address, uint32_t offset = 0) { return 0; }

    virtual uint8_t readU8Live(uint32_t address, uint32_t offset = 0) { return 0; }
    virtual uint16_t readU16Live(uint32_t address, uint32_t offset = 0) { return 0; }
    virtual uint32_t readU32Live(uint32_t address, uint32_t offset = 0) { return 0; }
};
