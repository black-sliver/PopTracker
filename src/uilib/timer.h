#pragma once


#include <stdint.h>


namespace Ui {

typedef uint32_t tick_t;
typedef uint64_t microtick_t;

microtick_t getMicroTicks(); /// return elapsed time since start or boot in ms
tick_t getTicks(); /// return elapsed time since start or boot in us
bool elapsed(tick_t start, tick_t time);
bool elapsed(microtick_t start, microtick_t time);

} // namespace Ui
