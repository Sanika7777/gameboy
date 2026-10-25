#pragma once

#include "core/Vec2.hpp"
#include "input/Button.hpp"

#include <optional>

namespace nb::input {
struct Gesture {
    enum class Kind : std::uint8_t { Tap, SwipeUp, SwipeDown, SwipeLeft, SwipeRight, LongPress };
    Kind kind;
    Vec2i at;
};

class InputState {
public:
    [[nodiscard]] bool held(Button b) const noexcept { return held_.test(bit(b)); }
    [[nodiscard]] bool pressed(Button b) const noexcept { return pressed_.test(bit(b)); }
    [[nodiscard]] bool released(Button b) const noexcept { return released_.test(bit(b)); }
    [[nodiscard]] bool anyPressed() const noexcept { return pressed_.any(); }
    [[nodiscard]] ButtonSet heldSet() const noexcept { return held_; }
    [[nodiscard]] std::optional<Vec2i> lcdTap() const noexcept { return tap_; }
    [[nodiscard]] std::optional<Vec2i> lcdTouch() const noexcept { return touch_; }
    [[nodiscard]] std::optional<Gesture> gesture() const noexcept { return gesture_; }

    void begin(ButtonSet held, std::optional<Vec2i> tap, std::optional<Vec2i> touch,
               std::optional<Gesture> gesture) noexcept {
        pressed_ = held & ~held_;
        released_ = held_ & ~held;
        held_ = held;
        tap_ = tap;
        touch_ = touch;
        gesture_ = gesture;
    }
    void endStep() noexcept {
        pressed_.reset();
        released_.reset();
        tap_.reset();
        gesture_.reset();
    }

    void press(Button b) noexcept { begin(held_ | ButtonSet{}.set(bit(b)), {}, {}, {}); }
    void release(Button b) noexcept { begin(held_ & ~ButtonSet{}.set(bit(b)), {}, {}, {}); }

private:
    ButtonSet held_, pressed_, released_;
    std::optional<Vec2i> tap_, touch_;
    std::optional<Gesture> gesture_;
};

}
