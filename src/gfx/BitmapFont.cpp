#include "gfx/BitmapFont.hpp"

#include "io/TextReader.hpp"

namespace nb::gfx {

BitmapFont BitmapFont::load(const std::filesystem::path& file) {
    io::TextReader r(file);
    BitmapFont f;
    if (!r.next()) r.fail("empty font");
    {
        const auto t = r.tokens();
        if (t.size() != 3 || t[0] != "size") r.fail("expected: size <w> <h>");
        f.w_ = io::parseInt(r, t[1]);
        f.h_ = io::parseInt(r, t[2]);
        if (f.w_ < 1 || f.w_ > 8 || f.h_ < 1 || f.h_ > 8) r.fail("glyph size must be 1..8");
    }
    // Raw reads throughout: '#' is both the comment marker and the "on" pixel.
    while (r.nextRaw()) {
        const auto line = r.line();
        if (line.empty() || line.starts_with("# ") || line == "#") continue;
        if (!line.starts_with("glyph ")) r.fail("expected: glyph <char>");
        const auto name = line.substr(6);
        const char c = name == "space" ? ' ' : (name.size() == 1 ? name[0] : '\0');
        if (c <= 0) r.fail("glyph name must be one character or 'space'");
        const auto ci = static_cast<std::size_t>(c);
        for (int y = 0; y < f.h_; ++y) {
            if (!r.nextRaw()) r.fail("unexpected end of file");
            const auto row = r.line();
            if (static_cast<int>(row.size()) != f.w_) r.fail("row width must be " + std::to_string(f.w_));
            for (int x = 0; x < f.w_; ++x) {
                const char p = row[static_cast<std::size_t>(x)];
                if (p == '#') f.rows_[ci][static_cast<std::size_t>(y)] |= static_cast<std::uint8_t>(1u << x);
                else if (p != '.') r.fail(std::string("bad pixel '") + p + "'");
            }
        }
        if (!r.nextRaw() || r.line() != "end") r.fail("missing 'end'");
        f.has_.set(ci);
    }
    return f;
}

bool BitmapFont::pixel(char c, int x, int y) const noexcept {
    auto ci = static_cast<std::size_t>(static_cast<unsigned char>(c));
    if (ci >= 128 || !has_.test(ci)) {
        if (c >= 'a' && c <= 'z') ci = static_cast<std::size_t>(c - 'a' + 'A');
        if (ci >= 128 || !has_.test(ci)) ci = '?';
    }
    return (rows_[ci][static_cast<std::size_t>(y)] >> x) & 1u;
}

} // namespace nb::gfx
