#include "input/InputSources.hpp"

namespace nb::input {

namespace {

std::optional<Button> buttonFor(Key k) noexcept {
    switch (k) {
    case Key::Up: case Key::W: return Button::Up;
    case Key::Down: case Key::S: return Button::Down;
    case Key::Left: case Key::A: return Button::Left;
    case Key::Right: case Key::D: return Button::Right;
    case Key::X: case Key::K: return Button::A;
    case Key::Z: case Key::J: return Button::B;
    case Key::Enter: case Key::Escape: return Button::Start;
    case Key::RShift: case Key::Backspace: return Button::Select;
    default: return std::nullopt;
    }
}

} // namespace

void KeyboardSource::handle(const PlatformEvent& e) {
    const auto* k = std::get_if<KeyEvent>(&e);
    if (!k || k->repeat) return;
    if (const auto b = buttonFor(k->key)) held_.set(bit(*b), k->down);
}

TouchSource::Finger* TouchSource::find(std::int64_t id) noexcept {
    for (auto& f : fingers_)
        if (f.id == id) return &f;
    return nullptr;
}

void TouchSource::handle(const PlatformEvent& e) {
    const auto* t = std::get_if<FingerEvent>(&e);
    if (!t) return;
    Finger* f = find(t->id);
    if (t->phase == PointerPhase::Up) {
        if (f) {
            if (f->onLcd) gestures_.up();
            *f = Finger{};
        }
        return;
    }
    if (!f) {
        if (t->phase != PointerPhase::Down || !(f = find(-1))) return;   // all 10 slots busy
        f->id = t->id;
    }
    const Hit h = hits_.hit(t->phys);
    if (h.lcd) {
        if (t->phase == PointerPhase::Down) {
            f->onLcd = true;
            gestures_.down(*h.lcd);
        } else if (f->onLcd) {
            gestures_.move(*h.lcd);
        }
    }
    // Sliding off one button onto another re-targets the finger; the D-pad centre keeps
    // the last direction so a rolling thumb doesn't flicker to "nothing".
    if (h.dpadDeadZone) return;
    if (!f->onLcd || !h.lcd) f->button = h.button;
}

void TouchSource::contribute(ButtonSet& held) const {
    for (const auto& f : fingers_)
        if (f.id != -1 && f.button) held.set(bit(*f.button));
}

void MouseSource::handle(const PlatformEvent& e) {
    const auto* m = std::get_if<MouseEvent>(&e);
    if (!m) return;
    if (m->phase == PointerPhase::Up) {
        if (onLcd_) gestures_.up();
        down_ = onLcd_ = false;
        button_.reset();
        return;
    }
    if (m->phase == PointerPhase::Move && !down_) return;
    const Hit h = hits_.hit(m->phys);
    if (m->phase == PointerPhase::Down) {
        down_ = true;
        onLcd_ = h.lcd.has_value();
        if (onLcd_) gestures_.down(*h.lcd);
    } else if (onLcd_ && h.lcd) {
        gestures_.move(*h.lcd);
    }
    if (!h.dpadDeadZone && !onLcd_) button_ = h.button;
}

} // namespace nb::input
