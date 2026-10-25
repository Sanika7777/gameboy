#pragma once

// Role: STACKFALL, the falling-block puzzle (design §11.3).
// Demonstrates: static constexpr std::array rotation tables; 7-bag std::shuffle; std::deque next queue;
//               std::all_of line detection

#include "core/Counted.hpp"
#include "core/Grid.hpp"
#include "games/GridGame.hpp"

#include <array>
#include <deque>
#include <filesystem>
#include <vector>

namespace nb::games::stackfall {

enum class Kind : std::uint8_t { I, O, T, S, Z, J, L };
inline constexpr int kKinds = 7;

using Cells = std::array<Vec2i, 4>;
using Rotations = std::array<Cells, 4>;

// Box size each piece rotates in: rotation maps (x, y) → (n-1-y, x).
inline constexpr std::array<int, kKinds> kBox{4, 2, 3, 3, 3, 3, 3};

constexpr std::array<Rotations, kKinds> makeShapes() {
    constexpr std::array<Cells, kKinds> spawn{{
        {{{0, 1}, {1, 1}, {2, 1}, {3, 1}}},   // I
        {{{0, 0}, {1, 0}, {0, 1}, {1, 1}}},   // O
        {{{1, 0}, {0, 1}, {1, 1}, {2, 1}}},   // T
        {{{1, 0}, {2, 0}, {0, 1}, {1, 1}}},   // S
        {{{0, 0}, {1, 0}, {1, 1}, {2, 1}}},   // Z
        {{{0, 0}, {0, 1}, {1, 1}, {2, 1}}},   // J
        {{{2, 0}, {0, 1}, {1, 1}, {2, 1}}},   // L
    }};
    std::array<Rotations, kKinds> out{};
    for (int k = 0; k < kKinds; ++k) {
        const int n = kBox[static_cast<std::size_t>(k)];
        out[static_cast<std::size_t>(k)][0] = spawn[static_cast<std::size_t>(k)];
        for (int r = 1; r < 4; ++r)
            for (int i = 0; i < 4; ++i) {
                const Vec2i c = out[static_cast<std::size_t>(k)][static_cast<std::size_t>(r - 1)][static_cast<std::size_t>(i)];
                out[static_cast<std::size_t>(k)][static_cast<std::size_t>(r)][static_cast<std::size_t>(i)] = {n - 1 - c.y, c.x};
            }
    }
    return out;
}

// Computed at compile time: no tables to load, no runtime cost.
inline constexpr std::array<Rotations, kKinds> kShapes = makeShapes();

// Offsets tried, in order, when a rotation collides (y grows downward).
inline constexpr std::array<Vec2i, 6> kKicks{{{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {-1, -1}, {1, -1}}};
inline constexpr std::array<Vec2i, 6> kKicksI{{{0, 0}, {-1, 0}, {1, 0}, {-2, 0}, {2, 0}, {0, -1}}};

struct Tuning {
    std::vector<int> gravity;   // frames per row, by level
    int das = 10, arr = 2, lockDelay = 30, lockResets = 15;
    static Tuning load(const std::filesystem::path& file);   // throws ParseError
};

class StackfallGame final : public GridGame, private Counted<StackfallGame> {
public:
    static constexpr int kWidth = 10, kHeight = 22, kHidden = 2;

    explicit StackfallGame(GameContext& ctx);

    static const GameInfo& staticInfo();
    [[nodiscard]] const GameInfo& info() const override { return staticInfo(); }
    void reset() override;
    [[nodiscard]] bool isOver() const override { return state_ == State::Over; }
    [[nodiscard]] std::uint32_t score() const override { return score_; }

    [[nodiscard]] bool supportsSaveState() const override { return true; }
    void save(io::BinaryWriter& w) const override;
    void load(io::BinaryReader& r) override;

    [[nodiscard]] static std::uint32_t lineScore(int lines, int level) noexcept;

protected:
    void handleInput(const input::InputState& in) override;
    void updateWorld(Seconds dt) override;
    void drawWorld(gfx::Renderer& r) const override;
    void drawHud(gfx::Renderer& r) const override;

private:
    enum class State : std::uint8_t { Falling, Clearing, Dying, Over };
    struct Piece {
        Kind kind = Kind::I;
        int rot = 0;
        Vec2i pos;
    };

    [[nodiscard]] bool fits(const Piece& p) const;
    [[nodiscard]] Kind nextFromBag();
    void spawn(Kind k);
    bool shift(int dx);
    bool rotate(int dir);
    void lock();
    void collapse();
    void touchedAfterMove();
    [[nodiscard]] Piece ghost() const;
    [[nodiscard]] int gravityFrames() const;
    void drawBlock(gfx::Renderer& r, Vec2i at, Kind k, int px) const;

    Tuning tuning_;
    Grid<std::uint8_t> board_;           // 0 empty, 1 + Kind otherwise
    std::array<Kind, kKinds> bag_{};
    int bagPos_ = kKinds;
    std::deque<Kind> next_;              // pop the front, refill the back
    Piece cur_;
    std::optional<Kind> hold_;
    bool canHold_ = true;
    State state_ = State::Falling;
    std::vector<int> clearing_;
    int fallTimer_ = 0, lockTimer_ = 0, resets_ = 0, dasTimer_ = 0, dasDir_ = 0, stateFrames_ = 0;
    int lines_ = 0, level_ = 1;
    std::uint32_t score_ = 0;

    friend class nb::games::GameTestAccess;
};

} // namespace nb::games::stackfall
