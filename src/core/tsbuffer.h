#pragma once

#include <map>
#include <stdint.h>
#include <mutex>

/// <summary>
/// Threadsafe buffer class
/// </summary>
template<typename T>
class tsbuffer {
public:
    tsbuffer() = default;
    tsbuffer(const tsbuffer<T>&) = delete;

    //T& operator[] (const uint32_t& k)
    //{
    //    //std::scoped_lock lock(_mutex);
    //    std::scoped_lock lock(_mutex);
    //    return _data[k];
    //}

    bool read(uint32_t address, unsigned int length, void* out)
    {
        std::scoped_lock lock(_mutex);

        uint8_t* dst = (uint8_t*)out;
        bool isCached = true;

        for (size_t i = 0; i < length; i++) {
            if (has(address + i)) {
                dst[i] = _data[address + i];
            }
            else {
                dst[i] = 0;
                isCached = false;
            }
        }

        return isCached;
    }

    template<typename R>
    bool readInt(uint32_t addr, R& out)
    {
        std::scoped_lock lock(_mutex);

        out = 0;
        bool isCached = true;

        for (size_t n = 0; n < sizeof(R); n++) {
            uint32_t address = addr + sizeof(R) - n - 1;
            out <<= 8;
            if (has(address)) {
                out += _data[address];
            }
            else {
                out += 0;
                isCached = false;
            }
        }

        return isCached;
    }

    // Returns true if any data was changed.
    bool write(uint32_t address, unsigned int length, const char* in)
    {
        std::scoped_lock lock(_mutex);

        uint8_t* src = (uint8_t*)in;
        bool bChanged = false;

        if (!has(address)) {
            // first time caching this address
            bChanged = true;
        }

        // copy to cache
        for (size_t i = 0; i < length; i++) {
            bChanged |= (_data[address + i] != src[i]);

            _data[address + i] = src[i];
        }

        return bChanged;
    }

    void clear()
    {
        std::scoped_lock lock(_mutex);
        _data.clear();
    }

protected:
    bool has(uint32_t address)
    {
        return _data.find(address) != _data.end();
    }

    std::mutex _mutex;
    std::map<uint32_t, T> _data;
};
