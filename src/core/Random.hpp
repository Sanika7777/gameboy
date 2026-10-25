#pragma once

// Role: the only source of randomness; seedable so games and tests are deterministic.

#include <cstdint>
#include <random>
#include <sstream>
#include <string>

namespace nb {

class Random {
public:
    explicit Random(std::uint32_t seed = 1) : engine_(seed) {}

    void seed(std::uint32_t s) { engine_.seed(s); }

    // Inclusive range.
    [[nodiscard]] int range(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(engine_); }
    [[nodiscard]] float real(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(engine_); }
    [[nodiscard]] bool chance(float p) { return real(0.0f, 1.0f) < p; }

    [[nodiscard]] std::mt19937& engine() noexcept { return engine_; }

    // Text form of the full engine state, so save states resume the exact same sequence.
    [[nodiscard]] std::string state() const {
        std::ostringstream os;
        os << engine_;
        return os.str();
    }
    void setState(const std::string& s) {
        std::istringstream is(s);
        is >> engine_;
    }

private:
    std::mt19937 engine_;
};

} // namespace nb
