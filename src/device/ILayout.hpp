#pragma once

#include "core/Rect.hpp"

#include <memory>
#include <string_view>

namespace nb::device {
struct Controls {
    Vec2i dpad;
    Vec2i a, b;
    Rect select, start;
};

class ILayout {
public:
    virtual ~ILayout() = default;
    [[nodiscard]] virtual Vec2i base() const = 0;
    [[nodiscard]] virtual Vec2i surfaceFor(Vec2i phys, int scale) const = 0;
    [[nodiscard]] virtual Vec2i lcdOrigin(Vec2i surface) const = 0;
    [[nodiscard]] virtual Controls controls(Vec2i surface) const = 0;
    [[nodiscard]] virtual bool isPortrait() const = 0;
};

class PortraitLayout final : public ILayout {
public:
    [[nodiscard]] Vec2i base() const override { return {180, 300}; }
    [[nodiscard]] Vec2i surfaceFor(Vec2i phys, int scale) const override;
    [[nodiscard]] Vec2i lcdOrigin(Vec2i) const override { return {10, 20}; }
    [[nodiscard]] Controls controls(Vec2i surface) const override;
    [[nodiscard]] bool isPortrait() const override { return true; }
};

class LandscapeLayout final : public ILayout {
public:
    [[nodiscard]] Vec2i base() const override { return {320, 180}; }
    [[nodiscard]] Vec2i surfaceFor(Vec2i phys, int scale) const override;
    [[nodiscard]] Vec2i lcdOrigin(Vec2i surface) const override { return {(surface.x - 160) / 2, 18}; }
    [[nodiscard]] Controls controls(Vec2i surface) const override;
    [[nodiscard]] bool isPortrait() const override { return false; }
};

[[nodiscard]] std::unique_ptr<ILayout> makeLayout(std::string_view setting, Vec2i phys);

struct Viewport {
    Vec2i surface;
    int scale = 1;
    Vec2i offset;
};
[[nodiscard]] Viewport fit(const ILayout& layout, Vec2i phys);

}
