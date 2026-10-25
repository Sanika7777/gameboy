#include "TestHarness.hpp"
#include "GameTestAccess.hpp"
#include "TestWorld.hpp"

#include "games/silverball/TableLoader.hpp"

#include <fstream>

using namespace nb;
using namespace nb::games::silverball;
using A = games::GameTestAccess;

namespace {

std::filesystem::path table(const std::string& name, const std::string& text) {
    const auto p = std::filesystem::temp_directory_path() / name;
    std::ofstream(p) << text;
    return p;
}

const std::string kMinimal =
    "flipper side=left pivot=48,262 length=22\nflipper side=right pivot=112,262 length=22\nplunger x=151 y=272\n";

} // namespace

TEST("SILVERBALL ball never passes through a wall at maximum speed") {
    nbtest::TestWorld w;
    SilverballGame g(w.ctx);
    A::play(g);
    input::InputState in;
    for (const float dir : {-1.0f, 1.0f}) {
        auto& b = A::ball(g);
        b.pos = {80, 180};
        b.vel = {dir * Ball::kMaxSpeed, 0};
        for (int f = 0; f < 30; ++f) {
            g.step(in, kStep);
            CHECK(b.pos.x > 6.0f && b.pos.x < 146.0f);   // between the outer wall and the lane divider
            b.vel.y = 0;
            b.pos.y = 180;
        }
    }
}

TEST("SILVERBALL a flipper hit adds speed") {
    nbtest::TestWorld w;
    SilverballGame g(w.ctx);
    A::play(g);
    auto& b = A::ball(g);
    b.pos = {62, 264};   // resting on the left flipper, near its tip
    b.vel = {0, 0};
    input::InputState in;
    in.press(input::Button::Left);
    nbtest::run(g, in, 4);
    CHECK(b.vel.y < -150.0f);   // flung upward
}

TEST("SILVERBALL nudging three times within 5 s tilts") {
    nbtest::TestWorld w;
    SilverballGame g(w.ctx);
    A::play(g);
    input::InputState in;
    for (int i = 0; i < 3; ++i) {
        in.press(input::Button::Select);
        g.step(in, kStep);
        in.endStep();
        in.release(input::Button::Select);
        g.step(in, kStep);
    }
    CHECK(A::tilted(g));
}

TEST("SILVERBALL table parser rejects unknown keywords and missing attributes") {
    try {
        (void)loadTable(table("nb_bad1.tbl", kMinimal + "bumpr x=1 y=2 r=3\n"));
        CHECK(false);
    } catch (const ParseError& e) {
        CHECK(e.line() == 4);
        CHECK(e.context() == "unknown 'bumpr'");
    }
    CHECK_THROWS_AS(loadTable(table("nb_bad2.tbl", kMinimal + "bumper x=1 r=3\n")), ParseError);
    CHECK_THROWS_AS(loadTable(table("nb_bad3.tbl", "wall points=1,1\n")), ParseError);
    CHECK_THROWS_AS(loadTable(table("nb_bad4.tbl", "bumper x=1 y=2 r=3\n")), ParseError);   // no flippers
    const auto t = loadTable(std::filesystem::path(NB_ASSET_DIR) / "tables/silverball_classic.tbl");
    CHECK(t.targets.size() == 6u && t.rollovers.size() == 3u);
}

TEST("SILVERBALL save state round trip") {
    nbtest::TestWorld w;
    SilverballGame g(w.ctx);
    A::play(g);
    A::ball(g).pos = {70, 100};
    A::ball(g).vel = {12, -30};
    A::table(g).targets[0]->onHit(A::ball(g), g, Contact{});
    io::BinaryWriter out;
    g.save(out);
    SilverballGame h(w.ctx);
    io::BinaryReader in(out.data());
    h.load(in);
    CHECK(A::ball(h).pos == A::ball(g).pos);
    CHECK(A::table(h).targets[0]->isDown());
    CHECK(h.score() == g.score());
}
