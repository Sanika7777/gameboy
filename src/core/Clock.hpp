#pragma once

#include <chrono>

namespace nb {
inline constexpr double kStep = 1.0 / 60.0;

class Clock {
public:
    static constexpr double kMaxDelta = 0.25;

    [[nodiscard]] int advance() noexcept {
        const auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - last_).count();
        last_ = now;
        if (dt > kMaxDelta) dt = kMaxDelta;
        accumulator_ += dt;
        int steps = 0;
        while (accumulator_ >= kStep) {
            accumulator_ -= kStep;
            ++steps;
        }
        return steps;
    }

    void reset() noexcept {
        last_ = std::chrono::steady_clock::now();
        accumulator_ = 0.0;
    }

private:
    std::chrono::steady_clock::time_point last_ = std::chrono::steady_clock::now();
    double accumulator_ = 0.0;
};

}
