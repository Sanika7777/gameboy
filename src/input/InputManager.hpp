#pragma once

// Role: owns every input source and merges them into one InputState per frame.

#include "input/InputSources.hpp"

#include <memory>
#include <vector>

namespace nb::input {

class InputManager {
public:
    explicit InputManager(const IHitTest& hits) {
        sources_.reserve(3);
        sources_.push_back(std::make_unique<KeyboardSource>());
        sources_.push_back(std::make_unique<TouchSource>(hits, gestures_));
        sources_.push_back(std::make_unique<MouseSource>(hits, gestures_));
    }

    void handle(const PlatformEvent& e) {
        for (auto& s : sources_) s->handle(e);
    }

    // Once per frame, after all events are handled.
    void beginFrame() {
        gestures_.tick();
        ButtonSet held;
        for (const auto& s : sources_) s->contribute(held);
        state_.begin(held, gestures_.takeTap(), gestures_.touch(), gestures_.takeGesture());
    }
    void endStep() noexcept { state_.endStep(); }

    [[nodiscard]] const InputState& state() const noexcept { return state_; }

private:
    GestureRecognizer gestures_;   // declared first: the sources hold a reference to it
    std::vector<std::unique_ptr<IInputSource>> sources_;
    InputState state_;
};

} // namespace nb::input
