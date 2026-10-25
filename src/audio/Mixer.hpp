#pragma once

// Role: owns the voices, applies master volume and soft clipping, fills the device buffer.
// Demonstrates: unique_ptr ownership of polymorphic objects; noexcept audio callback; std::span

#include "audio/Channel.hpp"
#include "core/RingBuffer.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>

namespace nb::audio {

struct AudioCommand {
    enum class Kind : std::uint8_t { Play, Volume, StopAll };
    Kind kind = Kind::Play;
    SfxDef sfx{};
    int volume = 5;
};

class Mixer {
public:
    Mixer();

    // Game thread.
    void play(const SfxDef& d) noexcept { (void)commands_.push({AudioCommand::Kind::Play, d, 0}); }
    void setVolume(int v) noexcept { (void)commands_.push({AudioCommand::Kind::Volume, {}, v}); }
    void stopAll() noexcept { (void)commands_.push({AudioCommand::Kind::StopAll, {}, 0}); }

    // Audio thread. Never throws, never allocates.
    void render(std::span<std::int16_t> out) noexcept;

private:
    void apply(const AudioCommand& c) noexcept;

    std::array<std::unique_ptr<Channel>, 4> channels_;   // pulse, pulse, wave, noise
    RingBuffer<AudioCommand, 64> commands_;
    int master_ = 5;
    int nextPulse_ = 0;
};

} // namespace nb::audio
