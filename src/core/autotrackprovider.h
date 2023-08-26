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

    virtual bool readFromCache(void* out, uint32_t address, unsigned int length) = 0;
    virtual bool readUInt8FromCache(uint8_t& out, uint32_t address, uint32_t offset = 0) = 0;
    virtual bool readUInt16FromCache(uint16_t& out, uint32_t address, uint32_t offset = 0) = 0;
    virtual bool readUInt32FromCache(uint32_t& out, uint32_t address, uint32_t offset = 0) = 0;

    virtual uint8_t readU8Live(uint32_t address, uint32_t offset = 0)
    {
        // this is Autotracker:ReadU8, which is a live blocking call in EmoTracker
        uint8_t result = 0;

        // we don't want to read on the main thread, so we will read from the cache instead
        if (!readUInt8FromCache(result, address, offset)) {
            // packs may expect not to have to create a watch for this address,
            // so let's do it for them
            addWatch(address + offset, 1);
        }

        return result;
    }

    virtual uint16_t readU16Live(uint32_t address, uint32_t offset = 0)
    {
        // this is Autotracker:ReadU16, which is a live blocking call in EmoTracker
        uint16_t result = 0;

        // we don't want to read on the main thread, so we will read from the cache instead
        if (!readUInt16FromCache(result, address, offset)) {
            // packs may expect not to have to create a watch for this address,
            // so let's do it for them
            addWatch(address + offset, 2);
        }

        // EmoTracker's Autotracker:ReadU16 returns swapped bytes
        // let's do it here for backwards compatibility
        uint16_t swapped = ((result & 0xff) << 8) | ((result & 0xff00) >> 8);
        return swapped;
    }

    virtual uint32_t readU24Live(uint32_t address, uint32_t offset = 0)
    {
        // this would be equivalent to Autotracker:ReadU24,
        // but i don't think that actually exists in Emotracker
        uint32_t result = 0;

        // we don't want to read on the main thread, so we will read from the cache instead
        if (!readUInt32FromCache(result, address, offset)) {
            // packs may expect not to have to create a watch for this address,
            // so let's do it for them
            addWatch(address + offset, 4);
        }

        return result & 0xffffff;
    }

    virtual uint32_t readU32Live(uint32_t address, uint32_t offset = 0)
    {
        // this would be equivalent to Autotracker:ReadU32,
        // but i don't think that actually exists in Emotracker
        uint32_t result = 0;

        // we don't want to read on the main thread, so we will read from the cache instead
        if (!readUInt32FromCache(result, address, offset)) {
            // packs may expect not to have to create a watch for this address,
            // so let's do it for them
            addWatch(address + offset, 4);
        }

        return result;
    }
};
