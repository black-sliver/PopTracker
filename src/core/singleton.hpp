#pragma once
#include <mutex>

namespace pop {

template<class T>
class Singleton {
public:
    static T& get()
    {
        static std::once_flag flag;
        std::call_once(flag, []{instance();});
        return instance();
    }

private:
    static T& instance()
    {
        static T instance;
        return instance;
    }
};

} // namespace pop
