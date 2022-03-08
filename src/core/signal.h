#ifndef _CORE_SIGNAL_H
#define _CORE_SIGNAL_H

#include <deque>
#include <functional>
#include <algorithm>
#include <stdio.h>
#include <memory>

template <typename ... Args>
class Signal {
public:
    using O = void*;
    using F = std::function<void(void*, Args...)>;
    using PAIR = std::pair<O,F>;
    void emit(void* sender, Args... args) {
        auto modified = _modified;
        *modified = false;
        for (auto& slot: _slots) {
            slot.second(sender, args...);
            if (*modified) break;
        }
    }
    void operator+=(PAIR pair) {
        //TODO: pair.first->onDelete += ..._slots.erase...
        _slots.push_back(pair);
        *_modified = true;
    }
    void operator-=(O o) {
        _slots.erase(std::remove_if(_slots.begin(), _slots.end(), [o](PAIR const &pair) { return pair.first==o; }), _slots.end());
        *_modified = true;
    }
    void operator-=(F f) {
        _slots.erase(std::remove_if(_slots.begin(), _slots.end(), [f](PAIR const &pair) { return pair.second==f; }), _slots.end());
        *_modified = true;
    }
    Signal() {
        _modified = std::make_shared<bool>(false);
    }
    virtual ~Signal() {
        *_modified = true;
    }
private:
    std::deque<PAIR> _slots;
    std::shared_ptr<bool> _modified;
};

#endif // _CORE_SIGNAL_H
