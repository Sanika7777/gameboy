#pragma once

// Role: base for continuous-motion games (FLAPWING, SILVERBALL): fixed substeps and a camera.
// Demonstrates: final virtual method (updateWorld); unique_ptr ownership of game objects

#include "games/Game.hpp"
#include "games/GameObject.hpp"

#include <memory>
#include <vector>

namespace nb::games {

struct Camera {
    Vec2f pos;
    // Exponential smoothing toward a target: no overshoot, frame-rate independent enough at 60 Hz.
    void follow(Vec2f target, float smoothing) noexcept {
        pos.x += (target.x - pos.x) * smoothing;
        pos.y += (target.y - pos.y) * smoothing;
    }
    [[nodiscard]] Vec2i offset() const noexcept { return {static_cast<int>(pos.x), static_cast<int>(pos.y)}; }
};

class PhysicsGame : public Game {
protected:
    PhysicsGame(GameContext& ctx, int substeps) : Game(ctx), substeps_(substeps) {}

    // final: subclasses can't skip substepping; they implement simulate() instead.
    void updateWorld(Seconds dt) final {
        const Seconds sub{dt.count() / substeps_};
        for (int i = 0; i < substeps_; ++i) simulate(sub);
        afterSubsteps(dt);
    }
    virtual void simulate(Seconds subDt) = 0;
    virtual void afterSubsteps(Seconds) {}

    std::vector<std::unique_ptr<GameObject>> objects_;
    Camera camera_;

private:
    int substeps_;
};

} // namespace nb::games
