#pragma once

// Role: integer axis-aligned rectangle for clipping, hit zones and layout.
// Demonstrates: delegating constructor; constexpr constructors; operator overloading

#include "core/Vec2.hpp"

#include <algorithm>

namespace nb {

struct Rect {
    int x = 0, y = 0, w = 0, h = 0;

    constexpr Rect() = default;
    constexpr Rect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}
    constexpr Rect(int w_, int h_) : Rect(0, 0, w_, h_) {}

    [[nodiscard]] constexpr int right() const { return x + w; }
    [[nodiscard]] constexpr int bottom() const { return y + h; }
    [[nodiscard]] constexpr bool empty() const { return w <= 0 || h <= 0; }
    [[nodiscard]] constexpr bool contains(Vec2i p) const {
        return p.x >= x && p.y >= y && p.x < right() && p.y < bottom();
    }
    // Intersection; empty Rect{} when the two don't overlap.
    [[nodiscard]] constexpr Rect operator&(const Rect& o) const {
        const int l = std::max(x, o.x), t = std::max(y, o.y);
        const int r = std::min(right(), o.right()), b = std::min(bottom(), o.bottom());
        return (r > l && b > t) ? Rect{l, t, r - l, b - t} : Rect{};
    }
    constexpr bool operator==(const Rect&) const = default;
};

} // namespace nb
