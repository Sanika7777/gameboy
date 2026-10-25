#pragma once

// Role: one LCD pixel value: four shades, plus Clear for transparent sprite pixels.

#include <cstdint>

namespace nb::gfx {

enum class Shade : std::uint8_t { Paper = 0, Light = 1, Dark = 2, Ink = 3, Clear = 0xFF };

[[nodiscard]] constexpr Shade invert(Shade s) noexcept {
    return s == Shade::Clear ? s : static_cast<Shade>(3 - static_cast<int>(s));
}

} // namespace nb::gfx
