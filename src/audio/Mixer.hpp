#pragma once

#include "audio/Channel.hpp"
#include "core/RingBuffer.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <span>

namespace nb::audio {
struct AudioCommand {
    enum class Kind : std::uint8_t { Play, Volume };
    Kind kind = Kind::Play;
    SfxDef sfx{};
    int volume = 5;
};

class Mixer {
public:
    Mixer();

    void play(const SfxDef& d) noexcept { (void)commands_.push({AudioCommand::Kind::Play, d, 0}); }
    void setVolume(int v) noexcept { (void)commands_.push({AudioCommand::Kind::Volume, {}, v}); }

    void render(std::span<std::int16_t> out) noexcept;

private:
    void apply(const AudioCommand& c) noexcept;

    std::array<std::unique_ptr<Channel>, 4> channels_;
    RingBuffer<AudioCommand, 64> commands_;
    int master_ = 5;
    int nextPulse_ = 0;
};

}
