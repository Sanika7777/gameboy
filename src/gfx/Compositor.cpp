#include "gfx/Compositor.hpp"

#include <algorithm>

namespace nb::gfx {
namespace {
constexpr Rgba blend(Rgba now, Rgba before) noexcept {
    Rgba out = 0xFF000000u;
    for (int shift = 0; shift <= 16; shift += 8) {
        const Rgba a = (now >> shift) & 0xFF, b = (before >> shift) & 0xFF;
        out |= ((a * 166 + b * 90) >> 8) << shift;
    }
    return out;
}

}

void GhostingEffect::apply(std::span<Rgba> frame, std::span<const Rgba> previous) const noexcept {
    for (std::size_t i = 0; i < frame.size() && i < previous.size(); ++i) frame[i] = blend(frame[i], previous[i]);
}

std::unique_ptr<LcdEffect> makeLcdEffect(std::string_view name) {
    if (name == "ghosting") return std::make_unique<GhostingEffect>();
    if (name == "grid") return std::make_unique<PixelGridEffect>();
    return std::make_unique<NoEffect>();
}

Compositor::Compositor() : effect_(std::make_unique<NoEffect>()) {}

void Compositor::compose(const PixelBuffer& lcd, const Palette& palette, RgbaSurface& out, Vec2i at) {
    const auto shades = lcd.pixels();
    frame_.resize(shades.size());
    previous_.resize(shades.size());

    const int fade = fade_;
    std::transform(shades.begin(), shades.end(), frame_.begin(), [&](Shade s) {
        const int v = std::clamp((static_cast<int>(s) & 3) - fade, 0, 3);
        return palette[static_cast<Shade>(v)];
    });
    effect_->apply(frame_, previous_);
    std::copy(frame_.begin(), frame_.end(), previous_.begin());

    for (int y = 0; y < lcd.height(); ++y)
        std::copy_n(frame_.begin() + static_cast<std::ptrdiff_t>(y) * lcd.width(), lcd.width(),
                    &out(at.x, at.y + y));
}

}
