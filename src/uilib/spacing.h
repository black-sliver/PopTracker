#ifndef _UILIB_SPACING_H
#define _UILIB_SPACING_H

namespace Ui {

struct Spacing;

struct Spacing final {
    int left;
    int top;
    int right;
    int bottom;
    constexpr Spacing() : left(0), top(0), right(0), bottom(0) {}
    constexpr Spacing(int s) : left(s), top(s), right(s), bottom(s) {}
    constexpr Spacing(int x, int y) : left(x), top(y), right(x), bottom(y) {}
    constexpr Spacing(int l, int t, int r, int b) : left(l), top(t), right(t), bottom(b) {}
    bool operator==(const Spacing& other) const { return left==other.left && top==other.top && right==other.right && bottom==other.bottom; }
    bool operator!=(const Spacing& other) const { return !(*this == other); }
};

} // namespace Ui

#endif // _UILIB_SPACING_H
