#pragma once

// Role: the composed device face in true colour, ready for texture upload.
// Demonstrates: Rule of Zero

#include "core/Vec2.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace nb::gfx {

using Rgba = std::uint32_t;   // 0xAARRGGBB, matches SDL_PIXELFORMAT_ARGB8888

// Same job as PixelBuffer but std::vector owns the memory, so the compiler-generated
// copy, move and destructor are already correct.
class RgbaSurface {
public:
    void resize(Vec2i size) {
        size_ = size;
        px_.assign(static_cast<std::size_t>(size.x) * size.y, 0xFF000000u);
    }
    [[nodiscard]] Vec2i size() const noexcept { return size_; }
    Rgba& operator()(int x, int y) noexcept { return px_[static_cast<std::size_t>(y) * size_.x + x]; }
    [[nodiscard]] std::span<const Rgba> pixels() const noexcept { return px_; }
    [[nodiscard]] std::span<Rgba> pixels() noexcept { return px_; }

private:
    Vec2i size_;
    std::vector<Rgba> px_;
};

} // namespace nb::gfx
