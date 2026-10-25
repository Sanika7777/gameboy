#pragma once

// Role: FLAPWING, the one-tap flyer (design §11.4).
// Demonstrates: multiple + virtual inheritance in use (Bird, ParallaxLayer are GameObjects);
//               PhysicsGame substeps; std::deque recycled with std::rotate

#include "core/Counted.hpp"
#include "games/PhysicsGame.hpp"

#include <deque>
#include <memory>

namespace nb::games::flapwing {

struct Physics {
    static constexpr float kGravity = 600.0f;     // px/s²
    static constexpr float kFlap = -170.0f;       // px/s, set (not added) on flap
    static constexpr float kTerminal = 250.0f;    // px/s
    static constexpr int kGround = 128;           // LCD y of the ground line
};

class Bird final : public GameObject {
public:
    explicit Bird(const gfx::SpriteSheet& sprites) : Identifiable("bird"), sprites_(sprites) {}
    void update(Seconds dt) override;
    void draw(gfx::Renderer& r) const override;
    void flap() noexcept { vel.y = Physics::kFlap; }
    // Two pixels smaller than the sprite on every side: near misses feel fair.
    [[nodiscard]] Rect hitbox() const noexcept {
        return {static_cast<int>(pos.x) + 2, static_cast<int>(pos.y) + 2, 10 - 4, 8 - 4};
    }
    Vec2f pos{40, 60}, vel;

private:
    const gfx::SpriteSheet& sprites_;
};

class ParallaxLayer final : public GameObject {
public:
    enum class Look : std::uint8_t { Clouds, Hills, Ground };
    ParallaxLayer(Look look, float factor, const float& speed)
        : Identifiable("parallax"), look_(look), factor_(factor), speed_(speed) {}
    void update(Seconds dt) override { offset += speed_ * factor_ * dt.asFloat(); }
    void draw(gfx::Renderer& r) const override;
    float offset = 0;

private:
    Look look_;
    float factor_;
    const float& speed_;   // the game's current scroll speed
};

struct PipePair {
    float x = 0;
    int gapY = 64;     // centre of the gap
    int gap = 44;
    bool scored = false;
    static constexpr int kWidth = 16;
};

class FlapwingGame final : public PhysicsGame, private Counted<FlapwingGame> {
public:
    static constexpr int kPipes = 3;
    static constexpr float kSpacing = 80.0f;

    explicit FlapwingGame(GameContext& ctx);

    static const GameInfo& staticInfo();
    [[nodiscard]] const GameInfo& info() const override { return staticInfo(); }
    void reset() override;
    [[nodiscard]] bool isOver() const override { return state_ == State::Over; }
    [[nodiscard]] std::uint32_t score() const override { return score_; }
    [[nodiscard]] bool isPlaying() const override { return state_ != State::Ready; }

    [[nodiscard]] bool supportsSaveState() const override { return true; }
    void save(io::BinaryWriter& w) const override;
    void load(io::BinaryReader& r) override;

protected:
    void handleInput(const input::InputState& in) override;
    void simulate(Seconds subDt) override;
    void afterSubsteps(Seconds dt) override;
    void drawWorld(gfx::Renderer& r) const override;
    void drawHud(gfx::Renderer& r) const override;

private:
    enum class State : std::uint8_t { Ready, Flying, Falling, Over };

    void resetPipe(PipePair& p, float x);
    [[nodiscard]] bool collides() const;

    std::shared_ptr<const gfx::SpriteSheet> sprites_;
    Bird* bird_ = nullptr;                 // non-owning views into objects_
    ParallaxLayer* clouds_ = nullptr;
    ParallaxLayer* hills_ = nullptr;
    ParallaxLayer* ground_ = nullptr;
    std::deque<PipePair> pipes_;           // front() is the left-most
    float speed_ = 50.0f;
    State state_ = State::Ready;
    std::uint32_t score_ = 0;
    int frames_ = 0, overFrames_ = 0;

    friend class nb::games::GameTestAccess;
};

} // namespace nb::games::flapwing
