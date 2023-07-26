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

    T& operator[] (const uint32_t& k)
    {
        //std::scoped_lock lock(_mutex);
        std::scoped_lock lock(_mutex);
        return _data[k];
    }

    void read(uint32_t address, unsigned int length, void* out)
    {
        std::scoped_lock lock(_mutex);
        uint8_t* dst = (uint8_t*)out;
        for (size_t i = 0; i < length; i++) dst[i] = _data[address + i];
    }

    template<typename R>
    R readInt(uint32_t addr)
    {
        std::scoped_lock lock(_mutex);
        R res = 0;
        {
            for (size_t n = 0; n < sizeof(R); n++) {
                res <<= 8;
                res += _data[addr + sizeof(R) - n - 1];
            }
        }
        return res;
    }

    // Returns true if any data was changed.
    bool write(uint32_t address, unsigned int length, const char* in)
    {
        std::scoped_lock lock(_mutex);
        uint8_t* src = (uint8_t*)in;
        bool bChanged = false;

        for (size_t i = 0; i < length; i++) {
            if (!bChanged && _data[address + i] != src[i])
                bChanged = true;

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
    std::mutex _mutex;
    std::map<uint32_t, T> _data;
};
