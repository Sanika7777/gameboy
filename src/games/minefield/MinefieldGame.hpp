#pragma once

// Role: MINEFIELD, mine sweeping with direct touch on cells (design §11.2).
// Demonstrates: std::set (flags, ordered by Vec2's <=>); BFS with std::queue; std::iota + std::shuffle;
//               std::count_if; Strategy (LowerIsBetter score order)

#include "core/Counted.hpp"
#include "core/Grid.hpp"
#include "games/GridGame.hpp"

#include <array>
#include <memory>
#include <set>
#include <vector>

namespace nb::games::minefield {

enum class Difficulty : std::uint8_t { Rookie, Veteran, Expert };

struct Spec {
    const char* name;
    const char* key;
    Vec2i size;
    int mines;
};

inline constexpr std::array<Spec, 3> kSpecs{{
    {"ROOKIE", "minefield.rookie", {9, 9}, 10},
    {"VETERAN", "minefield.veteran", {16, 16}, 40},
    {"EXPERT", "minefield.expert", {30, 16}, 99},
}};

class MinefieldGame final : public GridGame, private Counted<MinefieldGame> {
public:
    explicit MinefieldGame(GameContext& ctx);

    static const GameInfo& staticInfo();
    [[nodiscard]] const GameInfo& info() const override { return staticInfo(); }
    void reset() override;
    [[nodiscard]] bool isOver() const override { return state_ == State::Over; }
    [[nodiscard]] std::uint32_t score() const override { return seconds(); }
    [[nodiscard]] std::optional<std::uint32_t> result() const override;
    [[nodiscard]] std::string scoreTableKey() const override { return spec().key; }
    [[nodiscard]] bool isPlaying() const override { return state_ != State::Setup; }

    [[nodiscard]] bool supportsSaveState() const override { return true; }
    void save(io::BinaryWriter& w) const override;
    void load(io::BinaryReader& r) override;

    void begin(Difficulty d);

protected:
    void handleInput(const input::InputState& in) override;
    void updateWorld(Seconds dt) override;
    void drawWorld(gfx::Renderer& r) const override;
    void drawHud(gfx::Renderer& r) const override;

private:
    struct Cell {
        std::uint8_t mine = 0, revealed = 0, adjacent = 0;
    };
    enum class State : std::uint8_t { Setup, Ready, Playing, Lost, Won, Over };

    [[nodiscard]] const Spec& spec() const noexcept { return kSpecs[static_cast<std::size_t>(diff_)]; }
    [[nodiscard]] std::uint32_t seconds() const noexcept { return static_cast<std::uint32_t>(std::min(elapsed_, 999.0)); }
    void reveal(Vec2i c);
    void placeMines(Vec2i safe);
    void flood(Vec2i start);
    void toggleFlag(Vec2i c);
    void chord(Vec2i c);
    void act(Vec2i c, bool flag);
    void moveCursor(Vec2i d);
    void layout();
    [[nodiscard]] int flagsAround(Vec2i c) const;

    std::shared_ptr<const gfx::SpriteSheet> sprites_;
    Grid<Cell> cells_;
    std::set<Vec2i> flags_;   // ordered set: deterministic iteration, Vec2's <=> is the key order
    std::vector<Vec2i> mines_;
    Difficulty diff_ = Difficulty::Rookie;
    State state_ = State::Setup;
    Vec2i cursor_, boom_{-1, -1};
    int scroll_ = 0, revealed_ = 0, endFrames_ = 0, heldFrames_ = 0, frames_ = 0;
    double elapsed_ = 0;
    bool flagMode_ = false, digging_ = false;

    friend class nb::games::GameTestAccess;
};

} // namespace nb::games::minefield
