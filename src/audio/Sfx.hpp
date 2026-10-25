#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace nb::audio {
enum class Voice : std::uint8_t { Pulse, Wave, Noise };
enum class Decay : std::uint8_t { None, Slow, Fast };

struct SfxDef {
    Voice voice = Voice::Pulse;
    float duty = 0.5f;
    float fromHz = 440.0f, toHz = 440.0f;
    int ms = 100;
    int volume = 12;
    Decay decay = Decay::Fast;
    bool shortNoise = false;
};

class SfxLibrary {
public:
    static SfxLibrary load(const std::filesystem::path& file);
    [[nodiscard]] const SfxDef* find(std::string_view name) const;
    [[nodiscard]] std::size_t size() const noexcept { return defs_.size(); }

private:
    std::map<std::string, SfxDef, std::less<>> defs_;
};

}
