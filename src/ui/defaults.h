#ifndef _UI_DEFAULTS_H
#define _UI_DEFAULTS_H


#include "../uilib/timer.h"


namespace Ui {

constexpr static char DEFAULT_FONT_NAME[] = "DejaVuSans-Bold.ttf";
constexpr static int DEFAULT_FONT_SIZE = 11; // actually ~10px
constexpr static tick_t DEFAULT_TOOLTIP_DELAY = 1000; // 1 sec

} // namespace Ui

#endif // _UI_DEFAULTS_H
