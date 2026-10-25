#pragma once

#include <cstdint>
#include <random>
#include <sstream>
#include <string>

namespace nb {
class Random {
public:
    explicit Random(std::uint32_t seed = 1) : engine_(seed) {}

    void seed(std::uint32_t s) { engine_.seed(s); }

    [[nodiscard]] int range(int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(engine_); }
    [[nodiscard]] float real(float lo, float hi) { return std::uniform_real_distribution<float>(lo, hi)(engine_); }

    [[nodiscard]] std::mt19937& engine() noexcept { return engine_; }

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

}
