#pragma once

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
class SdlPlatform final {
public:
    using AudioFn = void (*)(void* user, std::int16_t* out, int samples) noexcept;

    SdlPlatform();
    ~SdlPlatform();
    SdlPlatform(const SdlPlatform&) = delete;
    SdlPlatform& operator=(const SdlPlatform&) = delete;

    [[nodiscard]] std::span<const PlatformEvent> pumpEvents();
    [[nodiscard]] Vec2i physSize() const;

    void present(std::span<const std::uint32_t> pixels, Vec2i size, Vec2i offset, int scale, std::optional<Rect> grid);

    void openAudio(AudioFn fn, void* user);
    void pauseAudio(bool paused);
    void vibrate(int ms);

    [[nodiscard]] static std::filesystem::path assetDir();
    [[nodiscard]] static std::filesystem::path saveDir();
    [[nodiscard]] static std::unique_ptr<io::IPersistence> makePersistence();

    void pushEvent(const PlatformEvent& e) { events_.push_back(e); }

private:
    struct Init {
        Init();
        ~Init();
        Init(const Init&) = delete;
        Init& operator=(const Init&) = delete;
    } init_;
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

}
