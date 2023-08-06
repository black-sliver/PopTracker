#include "timer.h"
#include <stdint.h>
#include <time.h> 


static inline uint64_t _getMicroTicks()
{
    timespec tp;
    if (clock_gettime(CLOCK_MONOTONIC, &tp) != 0) return 0;
    uint64_t micros = tp.tv_sec; micros *= 1000000;
    micros += tp.tv_nsec/1000;
    return micros;
}


namespace Ui {

microtick_t getMicroTicks()
{
    static_assert(sizeof(decltype(_getMicroTicks())) == 
            sizeof(decltype(Ui::getMicroTicks())),
            "Unexpected integer width");
    return (microtick_t)_getMicroTicks();
}

tick_t getTicks()
{
    return (uint32_t)(getMicroTicks() / 1000);
}

bool elapsed(tick_t start, tick_t time)
{
    tick_t now = getTicks();
    return (now - start >= time);
}

bool elapsed(microtick_t start, microtick_t time)
{
    microtick_t now = getMicroTicks();
    return (now - start >= time);
}

} // namespace