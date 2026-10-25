#include "device/DeviceShell.hpp"

#include <algorithm>
#include <cmath>

namespace nb::device {

namespace {

using gfx::Rgba;
using input::Button;

constexpr std::array<CaseTheme, 5> kThemes{{
    {"brick", 0xFFB9B3A6, 0xFF8C8679, 0xFFD6D0C2, 0xFF3A3A44, 0xFF23232B, 0xFF2E2E36, 0xFF15151B, 0xFF3F3A6E, false},
    {"grape", 0xFF7B5AA6, 0xFF55397D, 0xFF9C7EC6, 0xFF2A2338, 0xFF17121F, 0xFF3A2C52, 0xFF1C1428, 0xFFE9E0F5, true},
    {"teal",  0xFF2F9C95, 0xFF1F6F6A, 0xFF5CC2BA, 0xFF1D2B2E, 0xFF101A1C, 0xFF243336, 0xFF0E1517, 0xFFE8F6F4, false},
    {"bone",  0xFFE9E3D3, 0xFFC3BBA6, 0xFFFAF7EE, 0xFF4A4A4A, 0xFF2C2C2C, 0xFF8A2E3B, 0xFF5E1D27, 0xFF4A4A4A, false},
    {"night", 0xFF2A2D35, 0xFF17191E, 0xFF40444F, 0xFF101114, 0xFF050506, 0xFF4C5260, 0xFF2C3038, 0xFFA9B0C0, false},
}};

// Small painter for the true-colour face; LCD drawing goes through gfx::Renderer instead.
class Painter {
public:
    explicit Painter(gfx::RgbaSurface& s) : s_(s) {}

    void fill(Rect r, Rgba c) {
        r = r & Rect(s_.size().x, s_.size().y);
        for (int y = r.y; y < r.bottom(); ++y)
            for (int x = r.x; x < r.right(); ++x) s_(x, y) = c;
    }
    void circle(Vec2i c, int r, Rgba col) {
        for (int y = -r; y <= r; ++y)
            for (int x = -r; x <= r; ++x)
                if (x * x + y * y <= r * r + r) put(c.x + x, c.y + y, col);
    }
    void pill(Rect r, Rgba c) {
        const int rad = r.h / 2;
        fill(Rect(r.x + rad, r.y, r.w - 2 * rad, r.h), c);
        circle({r.x + rad, r.y + rad}, rad, c);
        circle({r.right() - rad - 1, r.y + rad}, rad, c);
    }
    void text(const gfx::BitmapFont& f, std::string_view str, Vec2i at, Rgba c) {
        for (std::size_t i = 0; i < str.size(); ++i)
            for (int y = 0; y < f.glyphHeight(); ++y)
                for (int x = 0; x < f.glyphWidth(); ++x)
                    if (f.pixel(str[i], x, y)) put(at.x + static_cast<int>(i) * f.advance() + x, at.y + y, c);
    }
    void put(int x, int y, Rgba c) {
        if (x >= 0 && y >= 0 && x < s_.size().x && y < s_.size().y) s_(x, y) = c;
    }

private:
    gfx::RgbaSurface& s_;
};

int floorDiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

} // namespace

std::vector<std::string> DeviceShell::themeNames() {
    std::vector<std::string> out;
    for (const auto& t : kThemes) out.emplace_back(t.name);
    return out;
}

void DeviceShell::configure(std::unique_ptr<ILayout> layout, Vec2i phys) {
    layout_ = std::move(layout);
    phys_ = phys;
    vp_ = fit(*layout_, phys);
    lcd_ = layout_->lcdOrigin(vp_.surface);
    controls_ = layout_->controls(vp_.surface);
    face_.resize(vp_.surface);
    frame_.resize(vp_.surface);
    if (!theme_) theme_ = &kThemes[0];
    dirty_ = true;
}

void DeviceShell::setTheme(std::string_view name) {
    const auto it = std::find_if(kThemes.begin(), kThemes.end(), [&](const CaseTheme& t) { return name == t.name; });
    theme_ = it == kThemes.end() ? &kThemes[0] : &*it;
    dirty_ = true;
}

void DeviceShell::setLeftHanded(bool on) {
    if (on != leftHanded_) dirty_ = true;
    leftHanded_ = on;
}

void DeviceShell::setPressed(input::ButtonSet held) {
    if (held != pressed_) dirty_ = true;
    pressed_ = held;
}

void DeviceShell::setLed(bool on) {
    if (on != led_) dirty_ = true;
    led_ = on;
}

gfx::RgbaSurface& DeviceShell::beginFrame() {
    if (dirty_) redraw();
    std::copy(face_.pixels().begin(), face_.pixels().end(), frame_.pixels().begin());
    return frame_;
}

void DeviceShell::redraw() {
    dirty_ = false;
    const CaseTheme& t = *theme_;
    Painter p(face_);
    const Vec2i size = vp_.surface;

    // Case body.
    for (int y = 0; y < size.y; ++y)
        for (int x = 0; x < size.x; ++x)
            face_(x, y) = (t.dithered && ((x + y) & 1)) ? t.light : t.body;
    p.fill(Rect(0, 0, size.x, 1), t.light);
    p.fill(Rect(0, size.y - 2, size.x, 2), t.shadow);

    // Bezel, inner rim, LED.
    // Landscape keeps a thinner bottom lip so SELECT/START fit under it.
    const Rect bezel(lcd_.x - 6, lcd_.y - 8, 172, layout_->isPortrait() ? 164 : 157);
    p.fill(Rect(bezel.x + 1, bezel.y + 1, bezel.w, bezel.h), t.shadow);
    p.fill(bezel, t.bezel);
    p.fill(Rect(lcd_.x - 1, lcd_.y - 1, 162, 146), t.rim);
    p.circle({bezel.x + 5, bezel.y + 4}, 1, led_ ? 0xFFFF3B30 : 0xFF4A1A1A);
    p.text(mini_, "PWR", {bezel.x + 9, bezel.y + 2}, t.label == t.bezel ? t.light : 0xFF8A8A9A);

    // Wordmark.
    const std::string_view mark = "N E O B R I C K";
    const int markW = static_cast<int>(mark.size()) * mini_.advance();
    if (layout_->isPortrait()) {
        p.text(mini_, mark, {(size.x - markW) / 2, bezel.bottom() + 4}, t.label);
    } else {
        const int margin = (size.x - 160) / 2;
        p.text(mini_, "NEOBRICK", {size.x - margin / 2 - 16, 30}, t.label);
    }

    auto held = [&](Button b) { return pressed_.test(input::bit(b)); };

    // D-pad: shadow, cross, pressed arm darker and 1 px lower.
    const Vec2i d = mirror(controls_.dpad);
    p.fill(Rect(d.x - 19, d.y - 5, 39, 13), t.shadow);
    p.fill(Rect(d.x - 6, d.y - 18, 13, 39), t.shadow);
    p.fill(Rect(d.x - 19, d.y - 6, 39, 13), t.button);
    p.fill(Rect(d.x - 6, d.y - 19, 13, 39), t.button);
    struct Arm { Button b; Rect r; };
    const Arm arms[] = {{Button::Up, Rect(d.x - 6, d.y - 19, 13, 13)}, {Button::Down, Rect(d.x - 6, d.y + 7, 13, 13)},
                        {Button::Left, Rect(d.x - 19, d.y - 6, 13, 13)}, {Button::Right, Rect(d.x + 7, d.y - 6, 13, 13)}};
    for (const auto& arm : arms)
        if (held(arm.b)) p.fill(Rect(arm.r.x, arm.r.y + 1, arm.r.w, arm.r.h), t.buttonDown);
    p.circle(d, 3, t.buttonDown);

    // A / B.
    auto round = [&](Vec2i c, Button b, std::string_view label) {
        c = mirror(c);
        const bool down = held(b);
        if (!down) p.circle({c.x, c.y + 2}, 11, t.shadow);
        p.circle({c.x, c.y + (down ? 1 : 0)}, 11, down ? t.buttonDown : t.button);
        p.text(mini_, label, {c.x + 8, c.y + 10}, t.label);
    };
    round(controls_.a, Button::A, "A");
    round(controls_.b, Button::B, "B");

    // SELECT / START pills (not mirrored: they sit centred).
    auto pill = [&](Rect r, Button b, std::string_view label, bool labelLeft) {
        const bool down = held(b);
        if (!down) p.pill(Rect(r.x, r.y + 1, r.w, r.h), t.shadow);
        p.pill(Rect(r.x, r.y + (down ? 1 : 0), r.w, r.h), down ? t.buttonDown : t.button);
        const int lw = static_cast<int>(label.size()) * mini_.advance();
        if (layout_->isPortrait()) p.text(mini_, label, {r.x + (r.w - lw) / 2, r.bottom() + 3}, t.label);
        else p.text(mini_, label, {labelLeft ? r.x - lw - 2 : r.right() + 3, r.y}, t.label);
    };
    pill(controls_.select, Button::Select, "SELECT", true);
    pill(controls_.start, Button::Start, "START", false);

    // Speaker slots.
    const Vec2i sp = layout_->isPortrait() ? mirror({150, size.y - 26}) : Vec2i{size.x - 40, 160};
    for (int i = 0; i < 4; ++i)
        for (int k = 0; k < 10; ++k) p.put(sp.x + i * 5 + k / 2, sp.y + k, t.shadow);
}

input::Hit DeviceShell::hit(Vec2i phys) const {
    input::Hit h;
    const Vec2i s{floorDiv(phys.x - vp_.offset.x, vp_.scale), floorDiv(phys.y - vp_.offset.y, vp_.scale)};
    if (Rect(lcd_.x, lcd_.y, 160, 144).contains(s)) {
        h.lcd = s - lcd_;
        return h;
    }
    auto dist2 = [](Vec2i a, Vec2i b) { return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y); };

    // D-pad: direction from the angle, four 90° sectors; dead zone keeps the last direction.
    const Vec2i d = mirror(controls_.dpad);
    if (const int r2 = dist2(s, d); r2 <= 30 * 30) {
        if (r2 < 5 * 5) {
            h.dpadDeadZone = true;
            return h;
        }
        const int dx = s.x - d.x, dy = s.y - d.y;
        if (std::abs(dx) > std::abs(dy)) h.button = dx > 0 ? Button::Right : Button::Left;
        else h.button = dy > 0 ? Button::Down : Button::Up;
        return h;
    }
    // A/B: generous circles, nearest wins.
    const Vec2i a = mirror(controls_.a), b = mirror(controls_.b);
    const int da = dist2(s, a), db = dist2(s, b);
    if (std::min(da, db) <= 17 * 17) {
        h.button = da <= db ? Button::A : Button::B;
        return h;
    }
    // Pills: 32×20 zones around the drawn shape.
    auto zone = [](Rect r) { return Rect(r.x + r.w / 2 - 16, r.y + r.h / 2 - 10, 32, 20); };
    if (zone(controls_.select).contains(s)) h.button = Button::Select;
    else if (zone(controls_.start).contains(s)) h.button = Button::Start;
    return h;
}

} // namespace nb::device
