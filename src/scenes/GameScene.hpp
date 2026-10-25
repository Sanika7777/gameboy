#pragma once

#include "games/Game.hpp"
#include "scenes/Scene.hpp"
#include "scenes/Services.hpp"

#include <memory>
#include <string>

namespace nb::scenes {
class GameScene final : public Scene {
public:
    GameScene(Services& s, const std::string& gameId, bool resume);
    ~GameScene() override;

    void update(const input::InputState& in, double dt) override;
    void draw(gfx::Renderer& r) const override;
    void onHidden() override;

    void restart();
    void saveAndQuit();
    void quit();
    [[nodiscard]] const games::Game& game() const noexcept { return *game_; }

private:
    void finish();

    Services& s_;
    games::GameContext ctx_;
    std::unique_ptr<games::Game> game_;
    int overFrames_ = -1;
};

}
