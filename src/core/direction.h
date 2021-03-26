#ifndef _CORE_DIRECTION_H
#define _CORE_DIRECTION_H

#include <string>

enum class Direction {
    UNDEFINED,
    LEFT,
    RIGHT,
    UP,
    DOWN
};
static std::string to_string(Direction dir)
{
    switch (dir) {
        case Direction::LEFT: return "left";
        case Direction::RIGHT: return "right";
        case Direction::UP: return "up";
        case Direction::DOWN: return "down";
        default: return "undefined";
    }
}

#endif // _CORE_DIRECTION_H
