#pragma once

#include "audio/Mixer.hpp"
#include "audio/Sfx.hpp"
#include "core/Clock.hpp"
#include "core/Random.hpp"
#include "core/Signal.hpp"
#include "device/DeviceShell.hpp"
#include "games/GameRegistry.hpp"
#include "gfx/BitmapFont.hpp"
#include "gfx/Compositor.hpp"
#include "gfx/Renderer.hpp"
#include "input/InputManager.hpp"
#include "io/HighScoreTable.hpp"
#include "io/Settings.hpp"
#include "platform/SdlPlatform.hpp"
#include "scenes/Scene.hpp"
#include "scenes/Services.hpp"
#include "ui/Widget.hpp"

#include <memory>
#include <vector>

namespace nb {
class App final {
public:
    App();
    ~App();
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void frame() noexcept;
    [[nodiscard]] bool running() const noexcept { return running_; }

private:
    void start();
    void handle(const PlatformEvent& e);
    void relayout(Vec2i phys);
    void applySettings();
    void drawDebug();
    void showError(std::exception_ptr e) noexcept;
    static void renderAudio(void* self, std::int16_t* out, int samples) noexcept;

    platform::SdlPlatform platform_;
    std::unique_ptr<io::IPersistence> persistence_;
    gfx::BitmapFont mainFont_, miniFont_;
    gfx::PixelBuffer lcd_;
    gfx::Renderer renderer_;
    gfx::Compositor compositor_;
    device::DeviceShell shell_;
    input::InputManager input_;
    audio::Mixer mixer_;
    audio::SfxLibrary sfx_;
    io::Settings settings_;
    io::ScoreBook scores_;
    games::GameRegistry registry_;
    Random rng_;
    EventBus events_;
    scenes::SceneStack scenes_;
    scenes::Services services_;
    ui::Toast toast_;
    Clock clock_;
    std::vector<Connection> connections_;
    io::SettingsValues applied_;
    Vec2i phys_;
    int fadeFrames_ = 0;
    bool ready_ = false, running_ = true, visible_ = true, debug_ = false, firstLayout_ = true;
};

}
