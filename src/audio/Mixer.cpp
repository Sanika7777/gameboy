#include "audio/Mixer.hpp"

#include <cmath>

namespace nb::audio {

Mixer::Mixer()
    : channels_{std::make_unique<PulseChannel>(), std::make_unique<PulseChannel>(),
                std::make_unique<WaveChannel>(), std::make_unique<NoiseChannel>()} {}

void Mixer::apply(const AudioCommand& c) noexcept {
    switch (c.kind) {
    case AudioCommand::Kind::Volume: master_ = c.volume; break;
    case AudioCommand::Kind::StopAll:
        for (auto& ch : channels_) ch->stop();
        break;
    case AudioCommand::Kind::Play:
        switch (c.sfx.voice) {
        case Voice::Pulse:
            // Two pulse voices: alternate so a new blip doesn't cut off the previous one.
            channels_[static_cast<std::size_t>(nextPulse_)]->start(c.sfx);
            nextPulse_ ^= 1;
            break;
        case Voice::Wave: channels_[2]->start(c.sfx); break;
        case Voice::Noise: channels_[3]->start(c.sfx); break;
        }
        break;
    }
}

void Mixer::render(std::span<std::int16_t> out) noexcept {
    AudioCommand c;
    while (commands_.pop(c)) apply(c);
    const float gain = master_ / 7.0f * 0.6f;
    for (auto& s : out) {
        float mix = 0;
        for (auto& ch : channels_) mix += ch->render();
        mix *= gain;
        mix = mix / (1.0f + std::fabs(mix));   // soft clip: never wraps around, stays smooth
        s = static_cast<std::int16_t>(mix * 32000.0f);
    }
}

} // namespace nb::audio
