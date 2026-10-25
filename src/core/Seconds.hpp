#pragma once

// Role: strong type for durations so a frame count can't be passed where seconds are expected.
// Demonstrates: explicit constructor

#include <compare>

namespace nb {

class Seconds {
public:
    constexpr explicit Seconds(double v = 0.0) noexcept : v_(v) {}

    [[nodiscard]] constexpr double count() const noexcept { return v_; }
    [[nodiscard]] constexpr float asFloat() const noexcept { return static_cast<float>(v_); }

    constexpr Seconds& operator+=(Seconds o) noexcept { v_ += o.v_; return *this; }
    constexpr Seconds& operator-=(Seconds o) noexcept { v_ -= o.v_; return *this; }
    constexpr Seconds operator+(Seconds o) const noexcept { return Seconds{v_ + o.v_}; }
    constexpr Seconds operator-(Seconds o) const noexcept { return Seconds{v_ - o.v_}; }
    constexpr Seconds operator*(double k) const noexcept { return Seconds{v_ * k}; }
    constexpr auto operator<=>(const Seconds&) const = default;

private:
    double v_;
};

inline constexpr Seconds kStep{1.0 / 60.0};

} // namespace nb
