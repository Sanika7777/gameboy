#include "TestHarness.hpp"
#include "GameTestAccess.hpp"
#include "TestWorld.hpp"

#include "games/serpent/SerpentGame.hpp"

#include <fstream>

using namespace nb;
using A = games::GameTestAccess;
using games::serpent::SerpentGame;
using input::Button;

TEST("SERPENT grows when it eats") {
    nbtest::TestWorld w;
    SerpentGame g(w.ctx);
    g.begin(0, 5);
    const auto len = A::body(g).size();
    A::food(g) = A::head(g) + Vec2i{1, 0};
    A::stepOnce(g);
    CHECK(g.score() == 5u);
    A::stepOnce(g);
    A::stepOnce(g);
    CHECK(A::body(g).size() == len + 2);
}

TEST("SERPENT dies on the wall in Box mode and wraps in Wrap mode") {
    nbtest::TestWorld w;
    SerpentGame box(w.ctx);
    box.begin(0, 1);
    A::food(box) = {0, 0};
    for (int i = 0; i < 40 && !A::dead(box); ++i) A::stepOnce(box);
    CHECK(A::dead(box));
    SerpentGame wrap(w.ctx);
    wrap.begin(1, 1);
    A::food(wrap) = {0, 0};
    for (int i = 0; i < 35; ++i) A::stepOnce(wrap);
    CHECK(!A::dead(wrap));
    CHECK(A::head(wrap).x == (10 + 35) % 40);
}

TEST("SERPENT dies biting itself") {
    nbtest::TestWorld w;
    SerpentGame g(w.ctx);
    g.begin(0, 1);
    // A long body so that turning down-left-up runs into it.
    A::setBody(g, {{10, 10}, {9, 10}, {8, 10}, {7, 10}, {6, 10}, {5, 10}});
    input::InputState in;
    in.press(Button::Down);
    g.step(in, kStep);
    A::stepOnce(g);
    in.release(Button::Down);
    in.press(Button::Left);
    g.step(in, kStep);
    A::stepOnce(g);
    in.release(Button::Left);
    in.press(Button::Up);
    g.step(in, kStep);
    A::stepOnce(g);
    CHECK(A::dead(g));
}

TEST("SERPENT rejects 180-degree reversals and buffers at most two turns") {
    nbtest::TestWorld w;
    SerpentGame g(w.ctx);
    g.begin(0, 1);
    input::InputState in;
    in.press(Button::Left);   // reverse of Right: ignored
    g.step(in, kStep);
    CHECK(A::queued(g) == 0u);
    in.release(Button::Left);
    in.press(Button::Up);
    g.step(in, kStep);
    in.release(Button::Up);
    in.press(Button::Left);
    g.step(in, kStep);
    in.release(Button::Left);
    in.press(Button::Down);   // third turn: dropped
    g.step(in, kStep);
    CHECK(A::queued(g) == 2u);
}

TEST("SERPENT food placement is deterministic with a fixed seed") {
    nbtest::TestWorld w1(1234), w2(1234);
    SerpentGame a(w1.ctx), b(w2.ctx);
    a.begin(1, 3);
    b.begin(1, 3);
    CHECK(A::food(a) == A::food(b));
    nbtest::TestWorld w3(99);
    SerpentGame c(w3.ctx);
    c.begin(1, 3);
    CHECK(SerpentGame::alive() == 3);
}

TEST("SERPENT save state round trip") {
    nbtest::TestWorld w(7);
    SerpentGame g(w.ctx);
    g.begin(2, 4);
    for (int i = 0; i < 10; ++i) A::stepOnce(g);
    io::BinaryWriter out;
    g.save(out);
    SerpentGame h(w.ctx);
    io::BinaryReader in(out.data());
    h.load(in);
    CHECK(A::body(h) == A::body(g));
    CHECK(A::food(h) == A::food(g));
    CHECK(h.score() == g.score());
    CHECK(h.scoreTableKey() == "serpent.maze");
}

TEST("Maze parser reports bad grids with line numbers") {
    const auto p = std::filesystem::temp_directory_path() / "nb_maze.txt";
    std::ofstream(p) << "maze X\n" << std::string(40, '.') << "\n" << std::string(39, '.') << "\n";
    try {
        (void)games::serpent::loadMazes(p);
        CHECK(false);
    } catch (const ParseError& e) {
        CHECK(e.line() == 3);
    }
    CHECK(games::serpent::loadMazes(std::filesystem::path(NB_ASSET_DIR) / "levels/serpent_mazes.txt").size() == 3u);
}
