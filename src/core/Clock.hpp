#pragma once

// Role: turns wall-clock time into a whole number of fixed 1/60 s simulation steps.

#include "core/Seconds.hpp"

#include <chrono>

namespace nb {

class Clock {
public:
    static constexpr double kMaxDelta = 0.25;   // after a stall, don't fast-forward more than this

    // Steps due since the last call (usually 0, 1 or 2).
    [[nodiscard]] int advance() noexcept {
        const auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - last_).count();
        last_ = now;
        if (dt > kMaxDelta) dt = kMaxDelta;
        accumulator_ += dt;
        int steps = 0;
        while (accumulator_ >= kStep.count()) {
            accumulator_ -= kStep.count();
            ++steps;
        }
        return steps;
    }

    // Called when the page becomes visible again so hidden time isn't simulated.
    void reset() noexcept {
        last_ = std::chrono::steady_clock::now();
        accumulator_ = 0.0;
    }

private:
    std::chrono::steady_clock::time_point last_ = std::chrono::steady_clock::now();
    double accumulator_ = 0.0;
};

} // namespace nb
