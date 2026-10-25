#pragma once

#include "scenes/Scene.hpp"
#include "scenes/Services.hpp"
#include "ui/Widget.hpp"

namespace nb::scenes {
class GameScene;

class PauseScene final : public Scene {
public:
    PauseScene(Services& s, GameScene& host);
    void update(const input::InputState& in, double dt) override;
    void draw(gfx::Renderer& r) const override;
    [[nodiscard]] bool isOverlay() const override { return true; }

private:
    Services& s_;
    GameScene& host_;
    ui::ListView list_;
};

}
