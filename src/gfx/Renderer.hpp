#pragma once

#include "core/Rect.hpp"
#include "gfx/BitmapFont.hpp"
#include "gfx/PixelBuffer.hpp"
#include "gfx/Sprite.hpp"

#include <string_view>

namespace nb::gfx {
enum class Flip : std::uint8_t { None, Horizontal, Vertical };

enum class Dither : std::uint8_t { Quarter = 4, Half = 8, ThreeQuarter = 12 };

class Renderer {
public:
    explicit Renderer(PixelBuffer& target);

    void setFonts(const BitmapFont* main, const BitmapFont* mini) noexcept { main_ = main; mini_ = mini; }
    [[nodiscard]] const BitmapFont& font(Font f) const noexcept { return f == Font::Mini ? *mini_ : *main_; }
    [[nodiscard]] PixelBuffer& target() noexcept { return target_; }

    void clear(Shade s = Shade::Paper);
    void pixel(Vec2i p, Shade s);
    void line(Vec2i a, Vec2i b, Shade s);
    void rect(const Rect& r, Shade s);
    void fill(const Rect& r, Shade s);
    void fill(const Rect& r, Dither d, Shade a, Shade b);
    void circle(Vec2i c, int r, Shade s);
    void fillCircle(Vec2i c, int r, Shade s);
    void draw(const Sprite& s, Vec2i at, Flip f = Flip::None);
    void draw(const SpriteSheet& sheet, int frame, Vec2i at, Flip f = Flip::None);
    int text(std::string_view str, Vec2i at, const TextStyle& style = {});

    void setClip(const Rect& r) noexcept;
    void resetClip() noexcept;
    void setCamera(Vec2i offset) noexcept { camera_ = offset; }
    [[nodiscard]] Vec2i camera() const noexcept { return camera_; }

private:
    void plot(int x, int y, Shade s) noexcept;
    void span(int x0, int x1, int y, Shade s) noexcept;

    PixelBuffer& target_;
    Rect clip_;
    Vec2i camera_;
    const BitmapFont* main_ = nullptr;
    const BitmapFont* mini_ = nullptr;
};

}
