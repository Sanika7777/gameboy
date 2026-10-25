#pragma once

// Role: small numeric helpers shared by games and effects.
// Demonstrates: function templates

#include "core/Vec2.hpp"

namespace nb {

template<Arithmetic T>
[[nodiscard]] constexpr T lerp(T a, T b, float t) {
    return static_cast<T>(a + (b - a) * t);
}

// Moves `value` toward `target` by at most `step`, never overshooting.
template<Arithmetic T>
[[nodiscard]] constexpr T approach(T value, T target, T step) {
    if (value < target) return (target - value < step) ? target : static_cast<T>(value + step);
    return (value - target < step) ? target : static_cast<T>(value - step);
}

template<Arithmetic T>
[[nodiscard]] constexpr T clampTo(T v, T lo, T hi) {
    return v < lo ? lo : (hi < v ? hi : v);
}

} // namespace nb
