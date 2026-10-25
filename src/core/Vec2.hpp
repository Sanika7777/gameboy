#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <ostream>
#include <type_traits>

namespace nb {
template<class T>
concept Arithmetic = std::is_arithmetic_v<T>;

template<Arithmetic T>
struct Vec2 {
    T x{}, y{};

    constexpr Vec2() = default;
    constexpr Vec2(T x_, T y_) : x(x_), y(y_) {}

    constexpr Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    constexpr Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }
    constexpr Vec2 operator+(const Vec2& o) const { return {T(x + o.x), T(y + o.y)}; }
    constexpr Vec2 operator-(const Vec2& o) const { return {T(x - o.x), T(y - o.y)}; }
    constexpr Vec2 operator-() const { return {T(-x), T(-y)}; }
    constexpr Vec2 operator*(T k) const { return {T(x * k), T(y * k)}; }

    constexpr auto operator<=>(const Vec2&) const = default;

    friend std::ostream& operator<<(std::ostream& os, const Vec2& v) {
        return os << '(' << v.x << ", " << v.y << ')';
    }
};

using Vec2i = Vec2<int>;
using Vec2f = Vec2<float>;

}

template<>
struct std::hash<nb::Vec2i> {
    std::size_t operator()(const nb::Vec2i& v) const noexcept {
        const auto packed = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(v.x)) << 32) |
                            static_cast<std::uint32_t>(v.y);
        return std::hash<std::uint64_t>{}(packed);
    }
};
