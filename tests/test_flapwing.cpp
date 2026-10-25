#include "TestHarness.hpp"
#include "GameTestAccess.hpp"
#include "TestWorld.hpp"

using namespace nb;
using A = games::GameTestAccess;
using games::flapwing::FlapwingGame;
using games::flapwing::Physics;

TEST("FLAPWING gravity and flap stay within bounds") {
    nbtest::TestWorld w;
    FlapwingGame g(w.ctx);
    auto& bird = A::bird(g);
    bird.pos = {40, -50};
    for (int i = 0; i < 120; ++i) bird.update(kStep);
    CHECK(bird.vel.y <= Physics::kTerminal);
    CHECK(bird.vel.y == Physics::kTerminal);
    bird.flap();
    CHECK(bird.vel.y == Physics::kFlap);
    bird.pos = {40, -5};
    bird.flap();
    for (int i = 0; i < 5; ++i) bird.update(kStep);
    CHECK(bird.pos.y >= -8.0f);   // the ceiling holds
}

TEST("FLAPWING recycles pipes and keeps a constant count") {
    nbtest::TestWorld w;
    FlapwingGame g(w.ctx);
    A::fly(g);
    input::InputState in;
    for (int f = 0; f < 60 * 20; ++f) {
        // Keep the bird hovering in the middle so it never crashes.
        A::bird(g).pos.y = static_cast<float>(A::pipes(g).front().gapY - 4);
        A::bird(g).vel.y = 0;
        for (auto& p : A::pipes(g)) p.gapY = 64, p.gap = 60;
        g.step(in, kStep);
        CHECK(A::pipes(g).size() == static_cast<std::size_t>(FlapwingGame::kPipes));
    }
    CHECK(!A::crashed(g));
    CHECK(g.score() > 5u);
}

TEST("FLAPWING passing a pipe scores exactly once") {
    nbtest::TestWorld w;
    FlapwingGame g(w.ctx);
    A::fly(g);
    auto& pipes = A::pipes(g);
    pipes[0].x = 50;
    pipes[1].x = 400;
    pipes[2].x = 480;
    for (auto& p : pipes) p.gapY = 64, p.gap = 60;
    input::InputState in;
    for (int f = 0; f < 60; ++f) {
        A::bird(g).pos.y = 60;
        A::bird(g).vel.y = 0;
        g.step(in, kStep);
    }
    CHECK(g.score() == 1u);
}

TEST("FLAPWING crashes into the ground") {
    nbtest::TestWorld w;
    FlapwingGame g(w.ctx);
    A::fly(g);
    input::InputState in;
    nbtest::run(g, in, 240);
    CHECK(A::crashed(g));
}
