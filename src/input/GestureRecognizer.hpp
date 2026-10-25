#pragma once

// Role: turns pointer activity on the LCD into taps, swipes and long-presses.

#include "input/InputState.hpp"

#include <algorithm>
#include <cstdlib>
#include <utility>

namespace nb::input {

class GestureRecognizer {
public:
    static constexpr int kSwipeDistance = 12;   // LCD px
    static constexpr int kSwipeFrames = 15;     // 250 ms
    static constexpr int kLongPressFrames = 21; // 350 ms
    static constexpr int kStillDistance = 3;

    void down(Vec2i lcd) noexcept {
        active_ = true;
        decided_ = false;
        start_ = current_ = lcd;
        age_ = 0;
        tap_ = lcd;
    }
    void move(Vec2i lcd) noexcept {
        if (!active_) return;
        current_ = lcd;
        if (decided_ || age_ > kSwipeFrames) return;
        const Vec2i d = lcd - start_;
        if (std::abs(d.x) >= kSwipeDistance || std::abs(d.y) >= kSwipeDistance) {
            using K = Gesture::Kind;
            const K k = std::abs(d.x) > std::abs(d.y) ? (d.x > 0 ? K::SwipeRight : K::SwipeLeft)
                                                     : (d.y > 0 ? K::SwipeDown : K::SwipeUp);
            emit(k);
        }
    }
    void up() noexcept {
        if (active_ && !decided_ && moved() <= kStillDistance) emit(Gesture::Kind::Tap);
        active_ = false;
    }
    // Once per frame.
    void tick() noexcept {
        if (!active_) return;
        ++age_;
        if (!decided_ && age_ >= kLongPressFrames && moved() <= kStillDistance) emit(Gesture::Kind::LongPress);
    }

    [[nodiscard]] std::optional<Vec2i> takeTap() noexcept { return std::exchange(tap_, std::nullopt); }
    [[nodiscard]] std::optional<Gesture> takeGesture() noexcept { return std::exchange(gesture_, std::nullopt); }
    [[nodiscard]] std::optional<Vec2i> touch() const noexcept {
        return active_ ? std::optional<Vec2i>{current_} : std::nullopt;
    }

private:
    [[nodiscard]] int moved() const noexcept { return std::max(std::abs(current_.x - start_.x), std::abs(current_.y - start_.y)); }
    void emit(Gesture::Kind k) noexcept {
        decided_ = true;
        gesture_ = Gesture{k, start_};
    }

    bool active_ = false, decided_ = false;
    Vec2i start_, current_;
    int age_ = 0;
    std::optional<Vec2i> tap_;
    std::optional<Gesture> gesture_;
};

} // namespace nb::input
