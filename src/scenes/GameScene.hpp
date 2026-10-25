#pragma once

// Role: hosts one Game: START pauses, game over leads to score entry.
// Demonstrates: nested exceptions (start-up failures wrapped with std::throw_with_nested)

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

    void update(const input::InputState& in, Seconds dt) override;
    void draw(gfx::Renderer& r) const override;
    void onHidden() override;

    // Used by PauseScene.
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

} // namespace nb::scenes
