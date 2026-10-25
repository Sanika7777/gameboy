#include "gfx/Sprite.hpp"

#include "core/Error.hpp"
#include "io/TextReader.hpp"

#include <algorithm>

namespace nb::gfx {
SpriteSheet SpriteSheet::load(const std::filesystem::path& file) {
    io::TextReader r(file);
    SpriteSheet sheet;
    sheet.file_ = r.fileName();
    while (r.next()) {
        const auto t = r.tokens();
        if (t.size() != 4 || t[0] != "sprite") r.fail("expected: sprite <name> <w> <h>");
        const int w = io::parseInt(r, t[2]), h = io::parseInt(r, t[3]);
        if (w <= 0 || h <= 0 || w > 64 || h > 64) r.fail("bad sprite size");
        std::string name(t[1]);
        PixelBuffer px(w, h, Shade::Clear);
        for (int y = 0; y < h; ++y) {
            if (!r.nextRaw()) r.fail("unexpected end of file in '" + name + "'");
            const auto row = r.line();
            if (static_cast<int>(row.size()) != w) r.fail("row width must be " + std::to_string(w));
            for (int x = 0; x < w; ++x) {
                const char c = row[static_cast<std::size_t>(x)];
                if (c == '.') continue;
                if (c < '0' || c > '3') r.fail(std::string("bad pixel '") + c + "'");
                px(x, y) = static_cast<Shade>(c - '0');
            }
        }
        if (!r.next() || r.line() != "end") r.fail("missing 'end' after '" + name + "'");
        sheet.frames_.emplace_back(std::move(px));
        sheet.names_.push_back(std::move(name));
    }
    return sheet;
}

int SpriteSheet::indexOf(std::string_view name) const {
    const auto it = std::find(names_.begin(), names_.end(), name);
    if (it == names_.end()) throw AssetError("SPRITE MISSING", file_ + ": " + std::string(name));
    return static_cast<int>(it - names_.begin());
}

const Sprite& SpriteSheet::get(std::string_view name) const { return frames_[static_cast<std::size_t>(indexOf(name))]; }

}
