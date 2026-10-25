#include "device/ILayout.hpp"

#include <algorithm>

namespace nb::device {
Vec2i PortraitLayout::surfaceFor(Vec2i phys, int scale) const {
    return {180, std::clamp(phys.y / scale, 300, 400)};
}

Controls PortraitLayout::controls(Vec2i surface) const {
    const int top = surface.y - 124;
    return {{40, top + 44}, {148, top + 34}, {118, top + 50}, Rect(62, top + 96, 24, 8), Rect(96, top + 96, 24, 8)};
}

Vec2i LandscapeLayout::surfaceFor(Vec2i phys, int scale) const {
    return {std::clamp(phys.x / scale, 320, 420), 180};
}

Controls LandscapeLayout::controls(Vec2i surface) const {
    const int margin = (surface.x - 160) / 2;
    const int left = margin / 2, right = surface.x - margin / 2;
    const int mid = surface.x / 2;
    return {{left, 100}, {right + 13, 92}, {right - 13, 108}, Rect(mid - 24, 167, 20, 6), Rect(mid + 4, 167, 20, 6)};
}

std::unique_ptr<ILayout> makeLayout(std::string_view setting, Vec2i phys) {
    const bool portrait = setting == "portrait" || (setting != "landscape" && phys.y > phys.x);
    if (portrait) return std::make_unique<PortraitLayout>();
    return std::make_unique<LandscapeLayout>();
}

Viewport fit(const ILayout& layout, Vec2i phys) {
    const Vec2i base = layout.base();
    Viewport v;
    v.scale = std::max(1, std::min(phys.x / base.x, phys.y / base.y));
    v.surface = layout.surfaceFor(phys, v.scale);
    v.offset = {(phys.x - v.surface.x * v.scale) / 2, (phys.y - v.surface.y * v.scale) / 2};
    return v;
}

}
