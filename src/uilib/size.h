#ifndef _UILIB_SIZE_H
#define _UILIB_SIZE_H

#include "position.h"

namespace Ui {

struct Size;

struct Size final {
    int width;
    int height;
    constexpr Size(int w, int h) : width(w), height(h) {}
    constexpr Size() : width(0), height(0) {}
    static Size FromPosition(const Position& o) { return {o.left,o.top}; }
    static const Size UNDEFINED;
    bool operator==(const Size& other) const { return width==other.width && height==other.height; }
    bool operator!=(const Size& other) const { return !(*this == other); }
};

constexpr const Size Size::UNDEFINED = {-1,-1};

static Size operator||(const Size& a, const Size& b)
{
    return { a.width  > b.width  ? a.width  : b.width,
             a.height > b.height ? a.height : b.height };
}
static Size operator&&(const Size& a, const Size& b)
{
    return { a.width  <= b.width  ? a.width  : b.width,
             a.height <= b.height ? a.height : b.height };
}

static Position operator+(const Position& a, const Size& b)
{
    return { a.left+b.width, a.top+b.height };
}

static Position& operator+=(Position& a, const Size& b)
{
    a.left += b.width;
    a.top += b.height;
    return a;
}

static Size operator*(const Size& a, const float b)
{
    // TODO: mirror what SDL does for rounding with render scale
    return {
        static_cast<int>(std::round(static_cast<float>(a.width) * b)),
        static_cast<int>(std::round(static_cast<float>(a.height) * b)),
    };
}

static Size& operator*=(Size& a, const float b)
{
    // TODO: mirror what SDL does for rounding with render scale
    a.width = static_cast<int>(std::round(static_cast<float>(a.width) * b));
    a.height = static_cast<int>(std::round(static_cast<float>(a.height) * b));
    return a;
}

static Size operator/(const Size& a, const float b)
{
    // TODO: mirror what SDL does for rounding with render scale
    return {
        static_cast<int>(std::round(static_cast<float>(a.width) / b)),
        static_cast<int>(std::round(static_cast<float>(a.height) / b)),
    };
}

static Size& operator/=(Size& a, const float b)
{
    // TODO: mirror what SDL does for rounding with render scale
    a.width = static_cast<int>(std::round(static_cast<float>(a.width) / b));
    a.height = static_cast<int>(std::round(static_cast<float>(a.height) / b));
    return a;
}

} // namespace Ui

#endif // _UILIB_SIZE_H
