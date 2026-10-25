#pragma once

// Role: draws the console face (case, bezel, buttons) and answers "which control is here?".

#include "device/ILayout.hpp"
#include "gfx/BitmapFont.hpp"
#include "gfx/RgbaSurface.hpp"
#include "input/IHitTest.hpp"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace nb::device {

struct CaseTheme {
    const char* name;
    gfx::Rgba body, shadow, light, bezel, rim, button, buttonDown, label;
    bool dithered;   // Grape: a translucent look from a two-colour checker
};

class DeviceShell final : public input::IHitTest {
public:
    explicit DeviceShell(const gfx::BitmapFont& mini) : mini_(mini) {}

    void configure(std::unique_ptr<ILayout> layout, Vec2i phys);
    void setTheme(std::string_view name);
    void setLeftHanded(bool on);
    void setPressed(input::ButtonSet held);
    void setLed(bool on);

    [[nodiscard]] const Viewport& viewport() const noexcept { return vp_; }
    [[nodiscard]] Vec2i lcdOrigin() const noexcept { return lcd_; }
    [[nodiscard]] Vec2i physSize() const noexcept { return phys_; }
    // This frame's face (the cached case copied in); the compositor then draws the LCD into it.
    [[nodiscard]] gfx::RgbaSurface& beginFrame();

    [[nodiscard]] input::Hit hit(Vec2i phys) const override;

    [[nodiscard]] static std::vector<std::string> themeNames();

private:
    void redraw();
    [[nodiscard]] Vec2i mirror(Vec2i p) const noexcept { return leftHanded_ ? Vec2i{vp_.surface.x - 1 - p.x, p.y} : p; }

    const gfx::BitmapFont& mini_;
    std::unique_ptr<ILayout> layout_;
    Viewport vp_;
    Vec2i phys_, lcd_;
    Controls controls_{};
    const CaseTheme* theme_ = nullptr;
    bool leftHanded_ = false, led_ = true, dirty_ = true;
    input::ButtonSet pressed_;
    gfx::RgbaSurface face_, frame_;
};

} // namespace nb::device
