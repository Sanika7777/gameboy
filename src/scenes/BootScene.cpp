#include "scenes/BootScene.hpp"

#include "scenes/Common.hpp"
#include "scenes/MenuScene.hpp"

namespace nb::scenes {

using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

void BootScene::update(const input::InputState& in, Seconds) {
    ++frames_;
    if (y_ < 44) {
        y_ = std::min(44, y_ + 2);   // style guide: ≤ 2 px per frame
        if (y_ == 44) {
            landedAt_ = frames_;
            s_.sfx("boot1");
        }
    } else if (frames_ == landedAt_ + 10) {
        s_.sfx("boot2");
    }
    if (in.pressed(Button::Start) || in.pressed(Button::A) || in.lcdTap()) {
        s_.sfx("select");
        s_.scenes.replace<MenuScene>(s_);
    }
}

void BootScene::draw(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    // Wordmark: a solid brick under the letters.
    r.fill(Rect(28, y_ - 3, 104, 14), Shade::Ink);
    r.text("NEOBRICK", {80, y_}, TextStyle{}.shade(Shade::Paper).align(Align::Center));
    if (landedAt_ < 0) return;
    r.text("POCKET CONSOLE", {80, 62}, TextStyle{}.font(gfx::Font::Mini).shade(Shade::Dark).align(Align::Center));
    if ((frames_ / 30) % 2 == 0) r.text("PRESS START", {80, 84}, TextStyle{}.align(Align::Center));
    const auto mini = TextStyle{}.font(gfx::Font::Mini).shade(Shade::Dark).align(Align::Center);
    r.text("ARROWS/WASD: MOVE", {80, 108}, mini);
    r.text("X/K: A   Z/J: B", {80, 116}, mini);
    r.text("ENTER: START  SHIFT: SELECT", {80, 124}, mini);
}

} // namespace nb::scenes
