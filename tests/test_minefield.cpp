#include "TestHarness.hpp"
#include "GameTestAccess.hpp"
#include "TestWorld.hpp"

#include "games/minefield/MinefieldGame.hpp"

using namespace nb;
using A = games::GameTestAccess;
using games::minefield::Difficulty;
using games::minefield::MinefieldGame;

TEST("MINEFIELD first reveal is never a mine and its 3x3 is clear") {
    for (std::uint32_t seed = 1; seed <= 30; ++seed) {
        nbtest::TestWorld w(seed);
        MinefieldGame g(w.ctx);
        g.begin(Difficulty::Expert);
        const Vec2i c{static_cast<int>(seed % 30), static_cast<int>(seed % 16)};
        A::reveal(g, c);
        CHECK(!A::lost(g));
        CHECK(A::mineCount(g) == 99);
        CHECK(A::adjacent(g, c) == 0);
    }
}

TEST("MINEFIELD flood fill stops at numbers") {
    nbtest::TestWorld w;
    MinefieldGame g(w.ctx);
    A::rig(g, {{8, 8}});
    A::reveal(g, {0, 0});
    CHECK(A::revealed(g, {7, 7}));    // a "1" next to the mine: revealed, but not expanded
    CHECK(!A::revealed(g, {8, 8}));
    CHECK(A::won(g));                 // everything but the one mine is open
}

TEST("MINEFIELD chording opens neighbours only when flags match") {
    nbtest::TestWorld w;
    MinefieldGame g(w.ctx);
    A::rig(g, {{1, 0}, {4, 4}});
    A::reveal(g, {0, 1});              // a "1"
    CHECK(A::adjacent(g, {0, 1}) == 1);
    A::chord(g, {0, 1});               // no flag yet: nothing happens
    CHECK(!A::revealed(g, {0, 0}));
    A::flag(g, {1, 0});
    A::chord(g, {0, 1});
    CHECK(A::revealed(g, {0, 0}) && A::revealed(g, {1, 2}));
    CHECK(!A::lost(g));
}

TEST("MINEFIELD revealing a mine loses; result only counts wins") {
    nbtest::TestWorld w;
    MinefieldGame g(w.ctx);
    A::rig(g, {{2, 2}});
    A::reveal(g, {2, 2});
    CHECK(A::lost(g));
    CHECK(!g.result().has_value());
    CHECK(g.scoreTableKey() == "minefield.rookie");
}
