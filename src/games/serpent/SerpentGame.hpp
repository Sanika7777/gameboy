#pragma once

#include "core/Grid.hpp"
#include "games/GridGame.hpp"

#include <deque>
#include <memory>
#include <queue>
#include <string>
#include <unordered_set>
#include <vector>

namespace nb::games::serpent {
enum class Direction : std::uint8_t { Up, Down, Left, Right };

struct Maze {
    std::string name;
    Grid<std::uint8_t> walls;
    Vec2i start;
};

[[nodiscard]] std::vector<Maze> loadMazes(const std::filesystem::path& file);

class SerpentGame final : public GridGame {
public:
    static constexpr Vec2i kCells{40, 34};
    static constexpr int kMaxTurns = 2;

    explicit SerpentGame(GameContext& ctx);

    static const GameInfo& staticInfo();
    [[nodiscard]] const GameInfo& info() const override { return staticInfo(); }
    void reset() override;
    [[nodiscard]] bool isOver() const override { return state_ == State::Over; }
    [[nodiscard]] std::uint32_t score() const override { return score_; }
    [[nodiscard]] std::string scoreTableKey() const override;
    [[nodiscard]] bool isPlaying() const override { return state_ != State::Setup; }

    [[nodiscard]] bool supportsSaveState() const override { return true; }
    void save(io::BinaryWriter& w) const override;
    void load(io::BinaryReader& r) override;


    void begin(int mode, int level);

protected:
    void handleInput(const input::InputState& in) override;
    void updateWorld(double dt) override;
    void drawWorld(gfx::Renderer& r) const override;
    void drawHud(gfx::Renderer& r) const override;

private:
    enum class State : std::uint8_t { Setup, Playing, Dying, Over };

    void queueTurn(Direction d);
    void advance();
    void placeFood();
    [[nodiscard]] bool blocked(Vec2i cell) const;
    [[nodiscard]] Vec2i wrap(Vec2i c) const noexcept;
    [[nodiscard]] double stepInterval() const noexcept { return 0.20 - (level_ - 1) * 0.018; }
    [[nodiscard]] bool foodAhead() const noexcept;
    [[nodiscard]] int modeCount() const noexcept { return 2 + static_cast<int>(mazes_.size()); }
    [[nodiscard]] std::string_view modeName(int m) const;
    [[nodiscard]] bool wraps() const noexcept { return mode_ != 0; }

    gfx::SpriteSheet sprites_;
    std::vector<Maze> mazes_;
    State state_ = State::Setup;
    int mode_ = 0, level_ = 5, setupRow_ = 0;
    std::deque<Vec2i> body_;
    std::unordered_set<Vec2i> occupied_;
    std::queue<Direction> turns_;
    Direction dir_ = Direction::Right;
    Vec2i food_{-1, -1}, critter_{-1, -1};
    double critterLeft_ = 0, stepTimer_ = 0;
    int eaten_ = 0, grow_ = 0, dyingFrames_ = 0, frames_ = 0;
    std::uint32_t score_ = 0;

    friend class nb::games::GameTestAccess;
};

}
