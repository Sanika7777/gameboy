#pragma once

// Role: SILVERBALL, pinball on a table loaded from a file (design §11.5).
// Demonstrates: Collider polymorphism in use; std::priority_queue timers; std::list particles with
//               remove_if; std::accumulate bonus; std::deque nudges

#include "core/Counted.hpp"
#include "games/PhysicsGame.hpp"
#include "games/silverball/TableLoader.hpp"

#include <deque>
#include <list>
#include <queue>
#include <vector>

namespace nb::games::silverball {

class SilverballGame final : public PhysicsGame, public Rules, private Counted<SilverballGame> {
public:
    static constexpr int kBalls = 3;
    static constexpr double kBallSave = 10.0;

    explicit SilverballGame(GameContext& ctx);

    static const GameInfo& staticInfo();
    [[nodiscard]] const GameInfo& info() const override { return staticInfo(); }
    void reset() override;
    [[nodiscard]] bool isOver() const override { return state_ == State::Over; }
    [[nodiscard]] std::uint32_t score() const override { return score_; }

    [[nodiscard]] bool supportsSaveState() const override { return true; }
    void save(io::BinaryWriter& w) const override;
    void load(io::BinaryReader& r) override;

    // Rules: what table elements may do.
    void award(int points) override { score_ += static_cast<std::uint32_t>(points * multiplier_); }
    void sound(std::string_view s) override { sfx(s); }
    void spark(Vec2f at) override;
    void targetDown(char group) override;
    void rolloverLit() override;

protected:
    void handleInput(const input::InputState& in) override;
    void simulate(Seconds subDt) override;
    void afterSubsteps(Seconds dt) override;
    void drawWorld(gfx::Renderer& r) const override;
    void drawHud(gfx::Renderer& r) const override;

private:
    enum class State : std::uint8_t { Launch, Play, Bonus, Over };
    enum class Timer : std::uint8_t { BallSaveEnd, LightsOut };
    struct Timed {
        double at;
        Timer what;
        bool operator>(const Timed& o) const noexcept { return at > o.at; }
    };
    struct Particle {
        Vec2f pos, vel;
        int life;
    };

    void newBall();
    void drained();
    void resolve(TableElement& e, const Contact& c);
    void nudge();

    Table table_;
    Ball* ball_ = nullptr;   // owned by objects_
    State state_ = State::Launch;
    std::uint32_t score_ = 0, bonus_ = 0;
    int balls_ = kBalls, multiplier_ = 1, bonusFrames_ = 0;
    double elapsed_ = 0, ballSaveUntil_ = 0;
    bool tilted_ = false, ballSaveLit_ = false;
    int targetsHit_ = 0, rolloversHit_ = 0;
    // Min-heap: the soonest timer is always on top.
    std::priority_queue<Timed, std::vector<Timed>, std::greater<>> timers_;
    std::deque<double> nudges_;      // times of recent nudges, oldest first
    // std::list: sparks are born and die at random positions in the sequence; erasing from the
    // middle is O(1) and never moves the others. A vector with erase_if would be faster to
    // iterate (contiguous) and is usually the better default; the list is kept as the one
    // place to show the trade-off (design §11.5).
    std::list<Particle> sparks_;

    friend class nb::games::GameTestAccess;
};

} // namespace nb::games::silverball
