#pragma once

#include <bitset>
#include <cstddef>
#include <cstdint>

namespace nb::input {
enum class Button : std::uint8_t { Up, Down, Left, Right, A, B, Start, Select };

inline constexpr int kButtonCount = 8;
using ButtonSet = std::bitset<kButtonCount>;

[[nodiscard]] constexpr std::size_t bit(Button b) noexcept { return static_cast<std::size_t>(b); }

}
