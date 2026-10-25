#pragma once

// Role: an image made of shades (Clear = transparent) and a named collection of them.

#include "gfx/PixelBuffer.hpp"

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace nb::gfx {

class Sprite {
public:
    explicit Sprite(PixelBuffer pixels) noexcept : px_(std::move(pixels)) {}
    [[nodiscard]] int width() const noexcept { return px_.width(); }
    [[nodiscard]] int height() const noexcept { return px_.height(); }
    [[nodiscard]] Shade operator()(int x, int y) const noexcept { return px_(x, y); }

private:
    PixelBuffer px_;
};

// Loaded from a .spr file (design §10.6). Frames are addressed by index or by name.
class SpriteSheet {
public:
    static SpriteSheet load(const std::filesystem::path& file);   // throws AssetError

    [[nodiscard]] const Sprite& frame(int index) const { return frames_.at(static_cast<std::size_t>(index)); }
    [[nodiscard]] const Sprite& get(std::string_view name) const;  // throws AssetError
    [[nodiscard]] int indexOf(std::string_view name) const;         // throws AssetError
    [[nodiscard]] int size() const noexcept { return static_cast<int>(frames_.size()); }

private:
    std::vector<Sprite> frames_;
    std::vector<std::string> names_;
    std::string file_;
};

} // namespace nb::gfx
