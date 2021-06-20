#ifndef _UILIB_CURSOR_H
#define _UILIB_CURSOR_H

#include <SDL.h>

namespace Ui {

enum class Cursor {
    DEFAULT = SDL_SYSTEM_CURSOR_ARROW,
    IBEAM = SDL_SYSTEM_CURSOR_IBEAM,
    WAIT = SDL_SYSTEM_CURSOR_WAIT,
    HAND = SDL_SYSTEM_CURSOR_HAND,
    CUSTOM,
};

} // namespace Ui

#endif // _UILIB_CURSOR_H
