#pragma once

#include "games/Game.hpp"
#include "games/GameObject.hpp"

#include <memory>
#include <vector>

namespace nb::games {
struct Camera {
    Vec2f pos;
    void follow(Vec2f target, float smoothing) noexcept {
        pos.x += (target.x - pos.x) * smoothing;
        pos.y += (target.y - pos.y) * smoothing;
    }
    [[nodiscard]] Vec2i offset() const noexcept { return {static_cast<int>(pos.x), static_cast<int>(pos.y)}; }
};

class PhysicsGame : public Game {
protected:
    PhysicsGame(GameContext& ctx, int substeps) : Game(ctx), substeps_(substeps) {}

    void updateWorld(double dt) final {
        const double sub = dt / substeps_;
        for (int i = 0; i < substeps_; ++i) simulate(sub);
        afterSubsteps(dt);
    }
    virtual void simulate(double subDt) = 0;
    virtual void afterSubsteps(double) {}

    std::vector<std::unique_ptr<GameObject>> objects_;
    Camera camera_;

private:
    int substeps_;
};

}
