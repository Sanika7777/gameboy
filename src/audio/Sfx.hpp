#pragma once

// Role: one sound effect's parameters, and the library parsed from assets/audio/sfx.txt.

#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>

namespace nb::audio {

enum class Voice : std::uint8_t { Pulse, Wave, Noise };
enum class Decay : std::uint8_t { None, Slow, Fast };

// Trivially copyable on purpose: it travels through the lock-free command queue.
struct SfxDef {
    Voice voice = Voice::Pulse;
    float duty = 0.5f;          // pulse only
    float fromHz = 440.0f, toHz = 440.0f;
    int ms = 100;
    int volume = 12;            // 0-15
    Decay decay = Decay::Fast;
    bool shortNoise = false;    // noise only: 7-bit LFSR, metallic
};

class SfxLibrary {
public:
    static SfxLibrary load(const std::filesystem::path& file);   // throws AssetError
    [[nodiscard]] const SfxDef* find(std::string_view name) const;
    [[nodiscard]] std::size_t size() const noexcept { return defs_.size(); }

private:
    // Ordered map with transparent lookup: find() by string_view allocates nothing.
    std::map<std::string, SfxDef, std::less<>> defs_;
};

} // namespace nb::audio
