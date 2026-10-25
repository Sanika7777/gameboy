#include "ui/Widget.hpp"

namespace nb::ui {
using gfx::Shade;
using input::Button;

bool ListView::handle(const input::InputState& in) {
    if (in.pressed(Button::Down)) { ++*this; return true; }
    if (in.pressed(Button::Up)) { --*this; return true; }
    return false;
}

void ListView::draw(gfx::Renderer& r) const {
    const auto& f = r.font(gfx::Font::Main);
    for (int i = 0; i < count(); ++i) {
        const Vec2i at{bounds_.x + 8, bounds_.y + i * (f.lineHeight() + 3)};
        const auto& item = items_[static_cast<std::size_t>(i)];
        if (i == cursor_) {
            r.text(item, at, gfx::TextStyle{}.inverted());
            if ((frames_ / 15) % 2 == 0) r.text(">", {bounds_.x, at.y});
        } else {
            r.text(item, at, gfx::TextStyle{}.shade(i == disabled_ ? Shade::Light : Shade::Ink));
        }
    }
}

void Toast::draw(gfx::Renderer& r) const {
    if (frames_ <= 0) return;
    r.fill(Rect(0, 132, 160, 12), Shade::Ink);
    r.text(text_, {80, 135}, gfx::TextStyle{}.shade(Shade::Paper).align(gfx::Align::Center));
}

}
