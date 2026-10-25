#pragma once

// Role: the single friend class every game grants to tests (one definition for the whole test binary).
// Demonstrates: friend class

#include "games/flapwing/FlapwingGame.hpp"
#include "games/minefield/MinefieldGame.hpp"
#include "games/serpent/SerpentGame.hpp"
#include "games/silverball/SilverballGame.hpp"
#include "games/stackfall/StackfallGame.hpp"

#include <deque>

namespace nb::games {

class GameTestAccess {
public:
    // --- SERPENT
    using S = serpent::SerpentGame;
    static std::deque<Vec2i>& body(S& g) { return g.body_; }
    static void setBody(S& g, std::deque<Vec2i> b) {
        g.body_ = std::move(b);
        g.occupied_.clear();
        g.occupied_.insert(g.body_.begin(), g.body_.end());
    }
    static Vec2i& food(S& g) { return g.food_; }
    static Vec2i head(const S& g) { return g.body_.front(); }
    static bool dead(const S& g) { return g.state_ == S::State::Dying || g.state_ == S::State::Over; }
    static std::size_t queued(const S& g) { return g.turns_.size(); }
    static void stepOnce(S& g) { g.advance(); }

    // --- MINEFIELD
    using M = minefield::MinefieldGame;
    static void reveal(M& g, Vec2i c) { g.reveal(c); }
    static void chord(M& g, Vec2i c) { g.chord(c); }
    static void flag(M& g, Vec2i c) { g.toggleFlag(c); }
    static bool revealed(const M& g, Vec2i c) { return g.cells_(c).revealed != 0; }
    static int adjacent(const M& g, Vec2i c) { return g.cells_(c).adjacent; }
    static int mineCount(const M& g) { return static_cast<int>(g.mines_.size()); }
    static bool lost(const M& g) { return g.boom_.x >= 0; }
    static bool won(const M& g) { return g.state_ == M::State::Won; }
    // Replace the board with a hand-made Rookie board (mines at the given cells).
    static void rig(M& g, std::initializer_list<Vec2i> mines) {
        g.begin(minefield::Difficulty::Rookie);
        g.state_ = M::State::Playing;
        for (const Vec2i m : mines) {
            g.cells_(m).mine = 1;
            g.mines_.push_back(m);
        }
        for (int y = 0; y < 9; ++y)
            for (int x = 0; x < 9; ++x) {
                int n = 0;
                for (const Vec2i q : g.cells_.neighbours({x, y})) n += g.cells_(q).mine;
                g.cells_(x, y).adjacent = static_cast<std::uint8_t>(n);
            }
    }

    // --- STACKFALL
    using F = stackfall::StackfallGame;
    static Grid<std::uint8_t>& board(F& g) { return g.board_; }
    static void place(F& g, stackfall::Kind k, int rot, Vec2i pos) { g.cur_ = {k, rot, pos}; }
    static Vec2i pos(const F& g) { return g.cur_.pos; }
    static bool rotate(F& g, int dir) { return g.rotate(dir); }
    static stackfall::Kind draw(F& g) { return g.nextFromBag(); }
    static void lock(F& g) { g.lock(); }
    static void finishClear(F& g) { g.collapse(); }
    static int lines(const F& g) { return g.lines_; }
    static bool dying(const F& g) { return g.state_ == F::State::Dying; }
    static void spawn(F& g, stackfall::Kind k) { g.spawn(k); }

    // --- FLAPWING
    using W = flapwing::FlapwingGame;
    static flapwing::Bird& bird(W& g) { return *g.bird_; }
    static std::deque<flapwing::PipePair>& pipes(W& g) { return g.pipes_; }
    static void fly(W& g) { g.state_ = W::State::Flying; }
    static bool crashed(const W& g) { return g.state_ == W::State::Falling || g.state_ == W::State::Over; }

    // --- SILVERBALL
    using P = silverball::SilverballGame;
    static silverball::Ball& ball(P& g) { return *g.ball_; }
    static void play(P& g) { g.state_ = P::State::Play; }
    static silverball::Table& table(P& g) { return g.table_; }
    static bool tilted(const P& g) { return g.tilted_; }
};

} // namespace nb::games
