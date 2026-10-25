#pragma once

#include "gfx/Palette.hpp"
#include "gfx/PixelBuffer.hpp"
#include "gfx/RgbaSurface.hpp"

#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace nb::gfx {
class LcdEffect {
public:
    virtual ~LcdEffect() = default;
    virtual void apply(std::span<Rgba> frame, std::span<const Rgba> previous) const noexcept = 0;
    [[nodiscard]] virtual bool wantsPixelGrid() const noexcept { return false; }
};

class NoEffect final : public LcdEffect {
public:
    void apply(std::span<Rgba>, std::span<const Rgba>) const noexcept override {}
};

class GhostingEffect final : public LcdEffect {
public:
    void apply(std::span<Rgba> frame, std::span<const Rgba> previous) const noexcept override;
};

class PixelGridEffect final : public LcdEffect {
public:
    void apply(std::span<Rgba>, std::span<const Rgba>) const noexcept override {}
    [[nodiscard]] bool wantsPixelGrid() const noexcept override { return true; }
};

[[nodiscard]] std::unique_ptr<LcdEffect> makeLcdEffect(std::string_view name);

class Compositor {
public:
    Compositor();

    void setEffect(std::unique_ptr<LcdEffect> e) noexcept { effect_ = std::move(e); }
    [[nodiscard]] const LcdEffect& effect() const noexcept { return *effect_; }
    void setFade(int level) noexcept { fade_ = level; }

    void compose(const PixelBuffer& lcd, const Palette& palette, RgbaSurface& out, Vec2i at);

private:
    std::unique_ptr<LcdEffect> effect_;
    std::vector<Rgba> frame_, previous_;
    int fade_ = 0;
};

}
