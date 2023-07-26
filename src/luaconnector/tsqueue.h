#ifndef _TSQUEUE_H
#define _TSQUEUE_H

#include "common.h"

namespace LuaConnector {

namespace Net {

template<typename T>
class tsqueue {
public:
    tsqueue() = default;
    tsqueue(const tsqueue<T>&) = delete;
    virtual ~tsqueue() { clear(); }

    const T& front()
    {
        std::scoped_lock lock(muxQueue);
        return deqQueue.front();
    }

    const T& back()
    {
        std::scoped_lock lock(muxQueue);
        return deqQueue.back();
    }

    // Returns true if the queue was empty before you pushed this item
    bool push_back(const T& item)
    {
        std::scoped_lock lock(muxQueue);
        bool wasEmpty = deqQueue.empty();
        deqQueue.emplace_back(std::move(item));
        return wasEmpty;
    }

    void push_front(const T& item)
    {
        std::scoped_lock lock(muxQueue);
        return deqQueue.emplace_front(std::move(item));
    }

    bool empty()
    {
        std::scoped_lock lock(muxQueue);
        return deqQueue.empty();
    }

    size_t count()
    {
        std::scoped_lock lock(muxQueue);
        return deqQueue.count();
    }

    void clear()
    {
        std::scoped_lock lock(muxQueue);
        deqQueue.clear();
    }

    // Returns true if the queue is now empty
    bool remove_front()
    {
        std::scoped_lock lock(muxQueue);
        deqQueue.pop_front();
        bool isNowEmpty = deqQueue.empty();
        return isNowEmpty;
    }

    T pop_front()
    {
        std::scoped_lock lock(muxQueue);
        auto t = std::move(deqQueue.front());
        deqQueue.pop_front();
        return t;
    }

    T pop_back()
    {
        std::scoped_lock lock(muxQueue);
        auto t = std::move(deqQueue.back());
        deqQueue.pop_back();
        return t;
    }

protected:
    std::mutex muxQueue;
    std::deque<T> deqQueue;
};

} // namespace Net

} // namespace LuaConnector

#endif
