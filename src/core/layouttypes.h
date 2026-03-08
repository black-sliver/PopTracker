#ifndef _CORE_LAYOUTTYPES_H
#define _CORE_LAYOUTTYPES_H

#include <nlohmann/json.hpp>

namespace LayoutTypes {
enum class OptionalBool {
    Undefined=-1,
    False=0,
    True=1,
};
enum class Orientation {
    UNDEFINED=0,
    HORIZONTAL=1,
    VERTICAL=2,
};

struct Size final {
    int x;
    int y;
    constexpr Size(const int w, const int h) : x(w), y(h) {}
    constexpr Size() : x(0), y(0) {}
};

struct Spacing;
struct Spacing final {
    int left;
    int top;
    int right;
    int bottom;
    constexpr Spacing() : left(0), top(0), right(0), bottom(0) {}
    constexpr explicit Spacing(int s) : left(s), top(s), right(s), bottom(s) {}
    constexpr Spacing(int x, int y) : left(x), top(y), right(x), bottom(y) {}
    constexpr Spacing(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
    bool operator==(const Spacing& other) const { return left==other.left && top==other.top && right==other.right && bottom==other.bottom; }
    bool operator!=(const Spacing& other) const { return !(*this == other); }
    static const Spacing UNDEFINED;
};

static void from_json(const nlohmann::json& j, Size& s) {
    s = {-1, -1};
    if (j.contains("x")) {
        j.at("x").get_to(s.x);
    }
    if (j.contains("y")) {
        j.at("y").get_to(s.y);
    }
}

}
static const LayoutTypes::Spacing dfltSpc={5,5,5,5};

constexpr const LayoutTypes::Spacing LayoutTypes::Spacing::UNDEFINED = {INT_MIN, INT_MIN, INT_MIN, INT_MIN};

#endif