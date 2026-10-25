#pragma once

// Role: overlay menu over a paused game.

#include "scenes/Scene.hpp"
#include "scenes/Services.hpp"
#include "ui/Widget.hpp"

namespace nb::scenes {

class GameScene;

class PauseScene final : public Scene {
public:
    PauseScene(Services& s, GameScene& host);
    void update(const input::InputState& in, Seconds dt) override;
    void draw(gfx::Renderer& r) const override;
    [[nodiscard]] bool isOverlay() const override { return true; }

private:
    Services& s_;
    GameScene& host_;
    ui::Panel panel_;
    ui::ListView* list_;
};

} // namespace nb::scenes
