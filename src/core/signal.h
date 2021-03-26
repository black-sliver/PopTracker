#ifndef _CORE_SIGNAL_H
#define _CORE_SIGNAL_H

#include <deque>
#include <functional>
#include <algorithm>
#include <stdio.h>

template <typename ... Args>
class Signal {
public:
    using O = void*;
    using F = std::function<void(void*, Args...)>;
    using PAIR = std::pair<O,F>;
    void emit(void* sender, Args... args) {
        for (auto& slot: _slots) {
            slot.second(sender, args...);
        }
    }
    void operator+=(PAIR pair) {
        //TODO: pair.first->onDelete += ..._slots.erase...
        _slots.push_back(pair);
    }
    void operator-=(O o) {
        _slots.erase(std::remove_if(_slots.begin(), _slots.end(), [o](PAIR const &pair) { return pair.first==o; }), _slots.end());
    }
    void operator-=(F f) {
        _slots.erase(std::remove_if(_slots.begin(), _slots.end(), [f](PAIR const &pair) { return pair.second==f; }), _slots.end());
    }
private:
    std::deque<PAIR> _slots;
};

#endif // _CORE_SIGNAL_H
