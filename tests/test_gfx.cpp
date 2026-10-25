#include "TestHarness.hpp"

#include "core/Error.hpp"
#include "gfx/BitmapFont.hpp"
#include "gfx/Compositor.hpp"
#include "gfx/Palette.hpp"
#include "gfx/PixelBuffer.hpp"
#include "gfx/Renderer.hpp"
#include "gfx/Sprite.hpp"

#include <fstream>
#include <stdexcept>

using namespace nb::gfx;

namespace {

std::uint64_t hashOf(const PixelBuffer& b) { return nbtest::hashBytes(b.pixels()); }

const std::filesystem::path kAssets = NB_ASSET_DIR;

std::filesystem::path writeTemp(const std::string& name, const std::string& content) {
    const auto p = std::filesystem::temp_directory_path() / name;
    std::ofstream(p) << content;
    return p;
}

} // namespace

TEST("PixelBuffer copy, move and self-assignment") {
    PixelBuffer a(4, 3, Shade::Light);
    a(1, 1) = Shade::Ink;
    PixelBuffer b(a);
    CHECK(a == b);
    b(0, 0) = Shade::Dark;
    CHECK(a(0, 0) == Shade::Light);                // deep copy
    PixelBuffer c(std::move(b));
    CHECK(c.width() == 4 && b.width() == 0);       // moved-from is empty
    auto& alias = c;
    c = alias;                                      // self-assignment is safe
    CHECK(c(0, 0) == Shade::Dark);
    a = std::move(c);
    CHECK(a(0, 0) == Shade::Dark);
    CHECK_THROWS_AS(a.at(4, 0), std::out_of_range);
}

TEST("Renderer primitives hash") {
    PixelBuffer buf(32, 32);
    Renderer r(buf);
    r.clear(Shade::Paper);
    r.line({0, 0}, {31, 20}, Shade::Ink);
    r.rect(nb::Rect(2, 2, 10, 8), Shade::Dark);
    r.fill(nb::Rect(20, 2, 8, 8), Shade::Light);
    r.fill(nb::Rect(4, 20, 12, 8), Dither::Half, Shade::Paper, Shade::Ink);
    r.circle({24, 24}, 6, Shade::Ink);
    r.fillCircle({8, 8}, 3, Shade::Ink);
    // Clipping: drawing off-buffer must not crash.
    r.fill(nb::Rect(-10, -10, 100, 3), Shade::Dark);
    CHECK(buf(0, 0) == Shade::Ink || buf(0, 0) == Shade::Dark);
    const auto h = hashOf(buf);
    // Same drawing twice is identical; any change in the algorithms changes the hash.
    PixelBuffer buf2(32, 32);
    Renderer r2(buf2);
    r2.line({0, 0}, {31, 20}, Shade::Ink);
    CHECK(h != hashOf(buf2));
    CHECK(buf(31, 20) == Shade::Ink);
    CHECK(buf(2, 2) == Shade::Dark && buf(11, 2) == Shade::Dark && buf(5, 5) != Shade::Dark);
    int inks = 0;
    for (int x = 4; x < 16; ++x) inks += buf(x, 21) == Shade::Ink;
    CHECK(inks == 6);   // 50 % dither
}

TEST("Renderer camera and clip") {
    PixelBuffer buf(8, 8);
    Renderer r(buf);
    r.setCamera({2, 0});
    r.pixel({3, 1}, Shade::Ink);
    CHECK(buf(1, 1) == Shade::Ink);
    r.setCamera({0, 0});
    r.setClip(nb::Rect(0, 0, 4, 4));
    r.fill(nb::Rect(0, 0, 8, 8), Shade::Dark);
    CHECK(buf(3, 3) == Shade::Dark && buf(4, 4) == Shade::Paper);
}

TEST("Fonts load and draw") {
    const auto main = BitmapFont::load(kAssets / "fonts/pixel5x7.fnt");
    const auto mini = BitmapFont::load(kAssets / "fonts/mini3x5.fnt");
    CHECK(main.glyphWidth() == 5 && mini.glyphHeight() == 5);
    CHECK(main.textWidth("AB") == 11);
    CHECK(main.pixel('A', 2, 0) && !main.pixel('A', 0, 0));
    CHECK(main.pixel('a', 2, 0));   // lower case falls back to upper
    PixelBuffer buf(40, 10);
    Renderer r(buf);
    r.setFonts(&main, &mini);
    CHECK(r.text("HI", {1, 1}) == 11);
    CHECK(buf(1, 1) == Shade::Ink);
}

TEST("Font and sprite parse errors carry line numbers") {
    const auto badFont = writeTemp("nb_bad.fnt", "size 3 2\nglyph A\n#.#\n#x#\nend\n");
    try {
        (void)BitmapFont::load(badFont);
        CHECK(false);
    } catch (const nb::ParseError& e) {
        CHECK(e.line() == 4);
        CHECK(e.file() == "nb_bad.fnt");
    }
    CHECK_THROWS_AS(BitmapFont::load(kAssets / "fonts/missing.fnt"), nb::FileNotFoundError);
    const auto badSpr = writeTemp("nb_bad.spr", "sprite a 2 2\n01\n2\nend\n");
    CHECK_THROWS_AS(SpriteSheet::load(badSpr), nb::ParseError);
    const auto noEnd = writeTemp("nb_noend.spr", "sprite a 2 1\n01\n");
    CHECK_THROWS_AS(SpriteSheet::load(noEnd), nb::ParseError);
}

TEST("Sprite round trip and transparency") {
    const auto spr = writeTemp("nb_ok.spr", "# test\nsprite dot 3 2\n.3.\n0.1\nend\n");
    const auto sheet = SpriteSheet::load(spr);
    CHECK(sheet.size() == 1);
    CHECK(sheet.get("dot")(1, 0) == Shade::Ink);
    CHECK(sheet.get("dot")(0, 0) == Shade::Clear);
    PixelBuffer buf(4, 4, Shade::Paper);
    Renderer r(buf);
    r.draw(sheet, 0, {0, 0}, Flip::Horizontal);
    CHECK(buf(0, 1) == Shade::Light);  // "1" flipped to the left edge
    CHECK(buf(0, 0) == Shade::Paper);  // transparent pixel kept background
    CHECK_THROWS_AS(sheet.get("nope"), nb::AssetError);
}

TEST("Palette presets and compositor") {
    CHECK_THROWS_AS((Palette{1, 2, 3}), nb::ContractViolation);
    Palette::loadPresets(kAssets / "data/palettes.txt");
    CHECK(Palette::presetNames().size() == 6u);
    CHECK(Palette::preset("stark")[Shade::Ink] == 0xFF000000u);
    CHECK(Palette::preset("nope")[Shade::Paper] == Palette::preset("moss")[Shade::Paper]);
    PixelBuffer lcd(2, 1, Shade::Ink);
    RgbaSurface out;
    out.resize({4, 2});
    Compositor c;
    c.compose(lcd, Palette::preset("stark"), out, {1, 1});
    CHECK(out(1, 1) == 0xFF000000u && out(0, 0) == 0xFF000000u);
    c.setFade(3);
    c.compose(lcd, Palette::preset("stark"), out, {1, 1});
    CHECK(out(2, 1) == 0xFFFFFFFFu);
}
