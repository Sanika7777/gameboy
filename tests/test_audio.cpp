#include "TestHarness.hpp"

#include "audio/Mixer.hpp"
#include "audio/Sfx.hpp"
#include "core/Error.hpp"
#include "core/RingBuffer.hpp"

#include <algorithm>
#include <array>
#include <fstream>

using namespace nb;
using namespace nb::audio;

namespace {

std::filesystem::path sfxFile(const std::string& text) {
    const auto p = std::filesystem::temp_directory_path() / "nb_sfx.txt";
    std::ofstream(p) << text;
    return p;
}

} // namespace

TEST("RingBuffer empty, full and wrap-around") {
    RingBuffer<int, 4> rb;   // holds 3
    int v = 0;
    CHECK(rb.empty() && !rb.pop(v));
    CHECK(rb.push(1) && rb.push(2) && rb.push(3));
    CHECK(!rb.push(4));      // full: dropped, not overwritten
    CHECK(rb.pop(v) && v == 1);
    CHECK(rb.push(4));       // wraps past the end of the array
    CHECK(rb.pop(v) && v == 2);
    CHECK(rb.pop(v) && v == 3);
    CHECK(rb.pop(v) && v == 4);
    CHECK(rb.empty());
}

TEST("Envelope goes attack, decay, sustain, release, idle") {
    Envelope e;
    e.start(1, 1, 0.5f, 1);
    CHECK(e.stage() == Envelope::Stage::Attack);
    for (int i = 0; i < 400 && e.stage() == Envelope::Stage::Attack; ++i) (void)e.next();
    CHECK(e.stage() == Envelope::Stage::Decay);
    for (int i = 0; i < 400 && e.stage() == Envelope::Stage::Decay; ++i) (void)e.next();
    CHECK(e.stage() == Envelope::Stage::Sustain);
    CHECK(e.level() == 0.5f);
    e.noteOff();
    for (int i = 0; i < 400 && !e.idle(); ++i) (void)e.next();
    CHECK(e.idle() && e.level() == 0.0f);
}

TEST("Mixer renders sound for a command, then silence") {
    Mixer m;
    m.setVolume(7);
    std::array<std::int16_t, 1024> buf{};
    m.render(buf);
    CHECK(std::all_of(buf.begin(), buf.end(), [](std::int16_t s) { return s == 0; }));
    SfxDef d;
    d.fromHz = d.toHz = 440;
    d.ms = 20;
    m.play(d);
    m.render(buf);
    CHECK(std::any_of(buf.begin(), buf.end(), [](std::int16_t s) { return s != 0; }));
    for (int i = 0; i < 4; ++i) m.render(buf);   // 20 ms + release are long gone
    CHECK(std::all_of(buf.begin(), buf.end(), [](std::int16_t s) { return s == 0; }));
}

TEST("SfxLibrary parses the real file and rejects bad lines") {
    const auto lib = SfxLibrary::load(std::filesystem::path(NB_ASSET_DIR) / "audio/sfx.txt");
    CHECK(lib.find("eat") && lib.find("eat")->voice == Voice::Pulse);
    CHECK(lib.find("explode") && lib.find("explode")->voice == Voice::Noise);
    CHECK(lib.find("nope") == nullptr);
    CHECK_THROWS_AS(SfxLibrary::load(sfxFile("sfx a pulse from=440 ms=10\nsfx b drum from=1 ms=1\n")), ParseError);
    CHECK_THROWS_AS(SfxLibrary::load(sfxFile("sfx a pulse duty=33 from=440 ms=10\n")), ParseError);
    CHECK_THROWS_AS(SfxLibrary::load(sfxFile("sfx a pulse ms=10\n")), ParseError);   // missing from=
    CHECK_THROWS_AS(SfxLibrary::load(sfxFile("sfx a pulse from=440 ms=10\nsfx a wave from=1 ms=1\n")), ParseError);
    try {
        (void)SfxLibrary::load(sfxFile("# c\nsfx a pulse from=440 ms=10 vol=99\n"));
    } catch (const ParseError& e) {
        CHECK(e.line() == 2);
    }
}
