#include "audio/Channel.hpp"

#include <algorithm>
#include <cmath>

namespace nb::audio {

void Envelope::start(int attackTicks, int decayTicks, float sustain, int releaseTicks) noexcept {
    // Small but non-zero attack/release so notes never start or stop with a click.
    attackStep_ = 1.0f / std::max(1.0f, attackTicks * kTick);
    decayStep_ = decayTicks > 0 ? (1.0f - sustain) / (decayTicks * kTick) : 0.0f;
    releaseStep_ = 1.0f / std::max(64.0f, releaseTicks * kTick);
    sustain_ = sustain;
    level_ = 0;
    stage_ = Stage::Attack;
}

float Envelope::next() noexcept {
    switch (stage_) {
    case Stage::Idle: return 0;
    case Stage::Attack:
        level_ += attackStep_;
        if (level_ >= 1) { level_ = 1; stage_ = decayStep_ > 0 ? Stage::Decay : Stage::Sustain; }
        break;
    case Stage::Decay:
        level_ -= decayStep_;
        if (level_ <= sustain_) { level_ = sustain_; stage_ = Stage::Sustain; }
        break;
    case Stage::Sustain: break;
    case Stage::Release:
        level_ -= releaseStep_;
        if (level_ <= 0) { level_ = 0; stage_ = Stage::Idle; }
        break;
    }
    return level_;
}

void Channel::start(const SfxDef& d) noexcept {
    samplesLeft_ = static_cast<int>(d.ms * kSampleRate / 1000.0f);
    freq_ = d.fromHz;
    freqStep_ = (d.toHz - d.fromHz) / std::max(1, samplesLeft_);
    phase_ = 0;
    volume_ = d.volume / 15.0f;
    const int lengthTicks = std::max(1, d.ms * 64 / 1000);
    switch (d.decay) {
    case Decay::None: env_.start(0, 0, 1.0f, 1); break;
    case Decay::Slow: env_.start(0, lengthTicks, 0.0f, 1); break;
    case Decay::Fast: env_.start(0, std::max(1, lengthTicks / 2), 0.0f, 1); break;
    }
    onStart(d);
}

float Channel::advancePhase() noexcept {
    const float p = phase_;
    phase_ += freq_ / kSampleRate;
    phase_ -= std::floor(phase_);
    return p;
}

float Channel::render() noexcept {
    if (env_.idle()) return 0;
    if (samplesLeft_ > 0 && --samplesLeft_ == 0) env_.noteOff();
    freq_ += freqStep_;
    return sample() * env_.next() * volume_;
}

WaveChannel::WaveChannel() noexcept {
    // A soft triangle; different timbre from the pulses.
    for (std::size_t i = 0; i < table_.size(); ++i)
        table_[i] = static_cast<std::uint8_t>(i < 16 ? i : 31 - i);
}

float WaveChannel::sample() noexcept {
    const auto idx = static_cast<std::size_t>(advancePhase() * 32.0f) & 31u;
    return table_[idx] / 7.5f - 1.0f;
}

float NoiseChannel::sample() noexcept {
    // The LFSR is clocked at the channel "frequency"; higher = hissier.
    clock_ += freq_ / kSampleRate;
    while (clock_ >= 1.0f) {
        clock_ -= 1.0f;
        const auto bit = static_cast<std::uint16_t>((lfsr_ ^ (lfsr_ >> 1)) & 1u);
        lfsr_ = static_cast<std::uint16_t>((lfsr_ >> 1) | (bit << 14));
        if (shortMode_) lfsr_ = static_cast<std::uint16_t>((lfsr_ & ~(1u << 6)) | (bit << 6));
    }
    return (lfsr_ & 1u) ? 1.0f : -1.0f;
}

} // namespace nb::audio
