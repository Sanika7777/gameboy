#pragma once

#include "audio/Sfx.hpp"

#include <array>
#include <cstdint>

namespace nb::audio {
inline constexpr float kSampleRate = 22050.0f;

class Envelope {
public:
    static constexpr float kTick = kSampleRate / 64.0f;

    void start(int attackTicks, int decayTicks, float sustain, int releaseTicks) noexcept;
    void noteOff() noexcept { if (stage_ != Stage::Idle) stage_ = Stage::Release; }
    [[nodiscard]] float next() noexcept;
    [[nodiscard]] bool idle() const noexcept { return stage_ == Stage::Idle; }
    [[nodiscard]] float level() const noexcept { return level_; }

    enum class Stage : std::uint8_t { Idle, Attack, Decay, Sustain, Release };
    [[nodiscard]] Stage stage() const noexcept { return stage_; }

private:
    Stage stage_ = Stage::Idle;
    float level_ = 0, sustain_ = 1;
    float attackStep_ = 1, decayStep_ = 0, releaseStep_ = 1;
};

class Channel {
public:
    virtual ~Channel() = default;
    void start(const SfxDef& d) noexcept;
    [[nodiscard]] bool active() const noexcept { return !env_.idle(); }
    [[nodiscard]] float render() noexcept;

protected:
    [[nodiscard]] virtual float sample() noexcept = 0;
    virtual void onStart(const SfxDef&) noexcept {}
    [[nodiscard]] float advancePhase() noexcept;

    float freq_ = 440, freqStep_ = 0, phase_ = 0;

private:
    Envelope env_;
    float volume_ = 0;
    int samplesLeft_ = 0;
};

class PulseChannel final : public Channel {
protected:
    void onStart(const SfxDef& d) noexcept override { duty_ = d.duty; }
    [[nodiscard]] float sample() noexcept override { return advancePhase() < duty_ ? 1.0f : -1.0f; }

private:
    float duty_ = 0.5f;
};

class WaveChannel final : public Channel {
public:
    WaveChannel() noexcept;

protected:
    [[nodiscard]] float sample() noexcept override;

private:
    std::array<std::uint8_t, 32> table_{};
};

class NoiseChannel final : public Channel {
protected:
    void onStart(const SfxDef& d) noexcept override { shortMode_ = d.shortNoise; lfsr_ = 0x7FFF; }
    [[nodiscard]] float sample() noexcept override;

private:
    std::uint16_t lfsr_ = 0x7FFF;
    bool shortMode_ = false;
    float clock_ = 0;
};

}
