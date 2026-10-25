#include "TestHarness.hpp"
#include "TestWorld.hpp"

#include "core/Signal.hpp"
#include "games/AllGames.hpp"
#include "games/GameRegistry.hpp"
#include "scenes/ErrorScene.hpp"
#include "scenes/Scene.hpp"

#include <string>
#include <vector>

using namespace nb;

namespace {

std::vector<std::string> trace;

struct Probe final : scenes::Scene {
    Probe(std::string n, bool overlay, scenes::SceneStack* stack = nullptr) : name(std::move(n)), overlay_(overlay), stack_(stack) {}
    void onEnter() override { trace.push_back("enter " + name); }
    void onExit() override { trace.push_back("exit " + name); }
    void update(const input::InputState&, Seconds) override {
        trace.push_back("update " + name);
        if (stack_) stack_->pop();   // remove myself mid-update: must be deferred
    }
    void draw(gfx::Renderer&) const override { trace.push_back("draw " + name); }
    [[nodiscard]] bool isOverlay() const override { return overlay_; }
    std::string name;
    bool overlay_;
    scenes::SceneStack* stack_;
};

} // namespace

TEST("SceneStack defers changes and draws overlays over the scene beneath") {
    trace.clear();
    scenes::SceneStack s;
    s.push<Probe>("menu", false);
    s.push<Probe>("game", false);
    CHECK(s.empty());   // nothing happens until applied
    s.applyPending();
    s.push<Probe>("pause", true, &s);
    s.applyPending();
    gfx::PixelBuffer buf(4, 4);
    gfx::Renderer r(buf);
    trace.clear();
    s.draw(r);
    CHECK((trace == std::vector<std::string>{"draw game", "draw pause"}));
    trace.clear();
    input::InputState in;
    s.update(in, kStep);
    CHECK((trace == std::vector<std::string>{"update pause", "exit pause"}));
    CHECK(s.size() == 2u);
}

TEST("GameRegistry rejects duplicates and unknown ids") {
    nbtest::TestWorld w;
    games::GameRegistry reg;
    games::registerAll(reg);
    CHECK(reg.catalog().size() >= 1u);
    CHECK_THROWS_AS(reg.add(reg.catalog().front(), nullptr), ContractViolation);
    CHECK_THROWS_AS(reg.create("nope", w.ctx), UnknownGameError);
    {
        auto g = reg.create("serpent", w.ctx);
        CHECK(games::Game::liveInstances() == 1);
    }
    CHECK(games::Game::liveInstances() == 0);
}

TEST("Signal disconnects when the Connection is destroyed") {
    Signal<int> sig;
    int sum = 0;
    {
        auto c = sig.connect([&](int v) { sum += v; });
        sig.emit(2);
        CHECK(sig.listenerCount() == 1u);
        auto moved = std::move(c);   // ownership moves; still connected
        sig.emit(3);
    }
    sig.emit(100);
    CHECK(sum == 5);
    CHECK(sig.listenerCount() == 0u);
}

TEST("ErrorScene unwinds nested exceptions and finds ParseError details") {
    nbtest::TestWorld w;
    scenes::SceneStack stack;
    io::NativePersistence persist;
    io::Settings settings(std::filesystem::temp_directory_path() / "nb_es.ini", persist);
    io::ScoreBook scores(std::filesystem::temp_directory_path() / "nb_es.dat", persist);
    games::GameRegistry reg;
    scenes::Services svc{stack, w.events, w.rng, w.assets, settings, scores, reg, persist};
    std::exception_ptr err;
    try {
        try {
            throw ParseError("classic.tbl", 14, "unknown 'bumpr'");
        } catch (const AssetError&) {
            std::throw_with_nested(GameError("CAN'T START SILVERBALL"));
        }
    } catch (...) {
        err = std::current_exception();
    }
    scenes::ErrorScene es(svc, err);
    const auto& l = es.lines();
    CHECK(l.size() == 5u);
    CHECK(l[0] == "CAN'T START SILVERBALL");
    CHECK(l[1] == "> PARSE ERROR");
    CHECK(l[2] == "> CLASSIC.TBL");
    CHECK(l[3] == "> LINE 14");
    CHECK(l[4] == "> UNKNOWN 'BUMPR'");
}

#include "scenes/SaveState.hpp"

TEST("Save states: round trip through the file, and newer versions are rejected") {
    nbtest::TestWorld w(3);
    io::NativePersistence persist;
    games::GameRegistry reg;
    games::registerAll(reg);
    for (const auto& info : reg.catalog()) {
        auto g = reg.create(info.id, w.ctx);
        CHECK(g->supportsSaveState());
        input::InputState in;
        in.press(input::Button::A);   // leave any setup screen
        nbtest::run(*g, in, 2);
        in.release(input::Button::A);
        nbtest::run(*g, in, 30);
        if (g->isOver()) continue;
        scenes::saveGame(*g, persist);
        CHECK(scenes::hasSave(info.id));
        auto h = reg.create(info.id, w.ctx);
        scenes::loadGame(*h);
        CHECK(h->score() == g->score());
        // Re-save the restored game: identical bytes mean identical state.
        io::BinaryWriter a, b;
        g->save(a);
        h->save(b);
        CHECK(a.data() == b.data());
        scenes::deleteSave(info.id, persist);
        CHECK(!scenes::hasSave(info.id));
    }
    // A .sav from a newer build: header version bumped (and CRC recomputed) → VersionMismatchError.
    auto g = reg.create("serpent", w.ctx);
    scenes::saveGame(*g, persist);
    const auto path = io::Paths::save("serpent.sav");
    auto bytes = io::readAll(path);
    bytes[4] = 99;
    io::BinaryWriter fixed;
    fixed.bytes(std::span(bytes).first(bytes.size() - 4));
    fixed.sealWithCrc();
    io::atomicWrite(path, fixed.data());
    CHECK_THROWS_AS(scenes::loadGame(*g), VersionMismatchError);
    bytes = io::readAll(path);
    bytes[8] ^= 0xFF;
    io::atomicWrite(path, bytes);
    CHECK_THROWS_AS(scenes::loadGame(*g), CorruptSaveError);
    scenes::deleteSave("serpent", persist);
}
