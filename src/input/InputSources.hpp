#pragma once

#include "core/PlatformEvent.hpp"
#include "input/GestureRecognizer.hpp"
#include "input/IHitTest.hpp"

#include <array>

namespace nb::input {
class IInputSource {
public:
    virtual ~IInputSource() = default;
    virtual void handle(const PlatformEvent& e) = 0;
    virtual void contribute(ButtonSet& held) const = 0;
};

class KeyboardSource final : public IInputSource {
public:
    void handle(const PlatformEvent& e) override;
    void contribute(ButtonSet& held) const override { held |= held_; }

private:
    ButtonSet held_;
};

class TouchSource final : public IInputSource {
public:
    TouchSource(const IHitTest& hits, GestureRecognizer& gestures) : hits_(hits), gestures_(gestures) {}
    void handle(const PlatformEvent& e) override;
    void contribute(ButtonSet& held) const override;

private:
    struct Finger {
        std::int64_t id = -1;
        std::optional<Button> button;
        bool onLcd = false;
    };
    Finger* find(std::int64_t id) noexcept;

    const IHitTest& hits_;
    GestureRecognizer& gestures_;
    std::array<Finger, 10> fingers_{};
};

class MouseSource final : public IInputSource {
public:
    MouseSource(const IHitTest& hits, GestureRecognizer& gestures) : hits_(hits), gestures_(gestures) {}
    void handle(const PlatformEvent& e) override;
    void contribute(ButtonSet& held) const override {
        if (button_) held.set(bit(*button_));
    }

private:
    const IHitTest& hits_;
    GestureRecognizer& gestures_;
    bool down_ = false, onLcd_ = false;
    std::optional<Button> button_;
};

}
