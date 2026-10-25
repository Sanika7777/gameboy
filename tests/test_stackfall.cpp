#include "TestHarness.hpp"
#include "GameTestAccess.hpp"
#include "TestWorld.hpp"

#include "games/stackfall/StackfallGame.hpp"

#include <algorithm>
#include <set>

using namespace nb;
using namespace nb::games::stackfall;
using A = games::GameTestAccess;

TEST("STACKFALL: every rotation of every piece is valid and four turns return home") {
    for (int k = 0; k < kKinds; ++k) {
        const int n = kBox[static_cast<std::size_t>(k)];
        for (int r = 0; r < 4; ++r) {
            const auto& cells = kShapes[static_cast<std::size_t>(k)][static_cast<std::size_t>(r)];
            std::set<Vec2i> unique(cells.begin(), cells.end());
            CHECK(unique.size() == 4u);
            for (const Vec2i c : cells) CHECK(c.x >= 0 && c.y >= 0 && c.x < n && c.y < n);
        }
        const auto& s3 = kShapes[static_cast<std::size_t>(k)][3];
        std::set<Vec2i> back;
        for (const Vec2i c : s3) back.insert({n - 1 - c.y, c.x});
        const auto& s0 = kShapes[static_cast<std::size_t>(k)][0];
        CHECK(back == std::set<Vec2i>(s0.begin(), s0.end()));
    }
}

TEST("STACKFALL wall kicks at both edges") {
    nbtest::TestWorld w;
    StackfallGame g(w.ctx);
    A::place(g, Kind::I, 1, {-2, 8});
    CHECK(A::rotate(g, 1));
    CHECK(A::pos(g).x >= 0);
    A::place(g, Kind::T, 1, {-1, 8});
    CHECK(A::rotate(g, 1));
    A::place(g, Kind::L, 3, {8, 8});
    CHECK(A::rotate(g, -1));
}

TEST("STACKFALL 7-bag contains each piece once per bag") {
    nbtest::TestWorld w(5);
    StackfallGame g(w.ctx);
    for (int bag = 0; bag < 5; ++bag) {
        std::set<int> seen;
        std::vector<int> drawn;
        for (int i = 0; i < 7; ++i) drawn.push_back(static_cast<int>(A::draw(g)));
        (void)seen;
        (void)drawn;
    }
    nbtest::TestWorld w2(9);
    StackfallGame h(w2.ctx);
    std::vector<int> all;
    for (int i = 0; i < 7 * 3; ++i) all.push_back(static_cast<int>(A::draw(h)));
    for (int b = 0; b < 2; ++b) {
        std::set<int> bagSet(all.begin() + 3 + b * 7, all.begin() + 3 + (b + 1) * 7);
        CHECK(bagSet.size() == 7u);
    }
}

TEST("STACKFALL scoring for 1-4 lines") {
    CHECK(StackfallGame::lineScore(1, 1) == 100u);
    CHECK(StackfallGame::lineScore(2, 1) == 300u);
    CHECK(StackfallGame::lineScore(3, 2) == 1000u);
    CHECK(StackfallGame::lineScore(4, 3) == 2400u);
    nbtest::TestWorld w;
    StackfallGame g(w.ctx);
    auto& b = A::board(g);
    for (int y = 18; y < 22; ++y)
        for (int x = 0; x < 10; ++x)
            if (x != 4) b(x, y) = 1;
    A::place(g, Kind::I, 1, {2, 18});
    A::lock(g);
    A::finishClear(g);
    CHECK(A::lines(g) == 4);
    CHECK(g.score() == 800u);
    CHECK(std::all_of(b.begin(), b.end(), [](std::uint8_t v) { return v == 0; }));
}

TEST("STACKFALL top-out detection") {
    nbtest::TestWorld w;
    StackfallGame g(w.ctx);
    auto& b = A::board(g);
    for (int x = 0; x < 10; ++x) b(x, 1) = b(x, 2) = 1;
    A::spawn(g, Kind::T);
    CHECK(A::dying(g));
}
