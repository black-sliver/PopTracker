#ifndef _UILIB_POSITION_H
#define _UILIB_POSITION_H

namespace Ui {

struct Position final {
    int left=0;
    int top=0;
    constexpr Position(int x, int y) : left(x), top(y) {}
    constexpr Position() : left(0), top(0) {}
};

static bool operator==(const Position& a, const Position& b) {
    return a.left == b.left && a.top == b.top;
}
static bool operator!=(const Position& a, const Position& b) {
    return !(a==b);
}

} // namespace Ui

#endif // _UILIB_POSITION_H
