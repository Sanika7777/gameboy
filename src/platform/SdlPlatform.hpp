#pragma once

// Role: the only window onto SDL/Emscripten: canvas, one texture, events, audio, haptics, paths.

#include "core/NonCopyable.hpp"
#include "core/PlatformEvent.hpp"
#include "core/Rect.hpp"
#include "io/IPersistence.hpp"
#include "platform/SdlHandles.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace nb::platform {

class SdlPlatform final : private NonCopyable {
public:
    using AudioFn = void (*)(void* user, std::int16_t* out, int samples) noexcept;

    SdlPlatform();
    ~SdlPlatform();

    // Events since the last call. The span stays valid until the next call.
    [[nodiscard]] std::span<const PlatformEvent> pumpEvents();
    [[nodiscard]] Vec2i physSize() const;

    // Draws the device face integer-scaled; `grid` (surface coords) gets 1-px LCD pixel gaps.
    void present(std::span<const std::uint32_t> pixels, Vec2i size, Vec2i offset, int scale, std::optional<Rect> grid);

    // 22,050 Hz mono 16-bit. Failure is logged, never fatal: the game runs silently.
    void openAudio(AudioFn fn, void* user);
    void pauseAudio(bool paused);
    void vibrate(int ms);

    [[nodiscard]] static std::filesystem::path assetDir();
    [[nodiscard]] static std::filesystem::path saveDir();
    [[nodiscard]] static std::unique_ptr<io::IPersistence> makePersistence();

    void pushEvent(const PlatformEvent& e) { events_.push_back(e); }   // from browser callbacks

private:
    struct Init {
        Init();
        ~Init();
        Init(const Init&) = delete;
        Init& operator=(const Init&) = delete;
    } init_;   // first member: SDL_Quit runs after every handle is freed
    SdlWindow window_;
    SdlRenderer renderer_;
    SdlTexture texture_;
    Vec2i textureSize_;
    std::vector<PlatformEvent> events_;
    std::size_t seen_ = 0;
    std::uint32_t audioDevice_ = 0;
    AudioFn audioFn_ = nullptr;
    void* audioUser_ = nullptr;
};

} // namespace nb::platform
