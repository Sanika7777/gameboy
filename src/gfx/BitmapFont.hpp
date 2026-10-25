#pragma once

// Role: a fixed-cell pixel font loaded from a .fnt file, plus the text style builder.
// Demonstrates: this and method chaining (TextStyle)

#include "gfx/Shade.hpp"

#include <array>
#include <bitset>
#include <cstdint>
#include <filesystem>
#include <string_view>

namespace nb::gfx {

class BitmapFont {
public:
    static BitmapFont load(const std::filesystem::path& file);   // throws AssetError

    [[nodiscard]] int glyphWidth() const noexcept { return w_; }
    [[nodiscard]] int glyphHeight() const noexcept { return h_; }
    [[nodiscard]] int advance() const noexcept { return w_ + 1; }
    [[nodiscard]] int lineHeight() const noexcept { return h_ + 1; }
    [[nodiscard]] int textWidth(std::string_view s) const noexcept {
        return s.empty() ? 0 : static_cast<int>(s.size()) * advance() - 1;
    }
    // Lower case falls back to upper case; unknown characters draw as '?'.
    [[nodiscard]] bool pixel(char c, int x, int y) const noexcept;

private:
    int w_ = 0, h_ = 0;
    std::array<std::array<std::uint8_t, 8>, 128> rows_{};   // bit x of rows_[c][y]
    std::bitset<128> has_;
};

enum class Font : std::uint8_t { Main, Mini };
enum class Align : std::uint8_t { Left, Center, Right };

// Fluent builder: TextStyle{}.font(Font::Mini).shade(Shade::Ink).align(Align::Center)
class TextStyle {
public:
    TextStyle& font(Font f) noexcept { font_ = f; return *this; }
    TextStyle& shade(Shade s) noexcept { shade_ = s; return *this; }
    TextStyle& align(Align a) noexcept { align_ = a; return *this; }
    TextStyle& inverted(bool on = true) noexcept { inverted_ = on; return *this; }

    [[nodiscard]] Font font() const noexcept { return font_; }
    [[nodiscard]] Shade shade() const noexcept { return shade_; }
    [[nodiscard]] Align align() const noexcept { return align_; }
    [[nodiscard]] bool inverted() const noexcept { return inverted_; }

private:
    Font font_ = Font::Main;
    Shade shade_ = Shade::Ink;
    Align align_ = Align::Left;
    bool inverted_ = false;
};

} // namespace nb::gfx
