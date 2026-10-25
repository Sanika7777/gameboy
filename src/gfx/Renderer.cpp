#include "gfx/Renderer.hpp"

#include <algorithm>
#include <cstdlib>

namespace nb::gfx {

namespace {

constexpr int kBayer[4][4] = {{0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5}};

} // namespace

Renderer::Renderer(PixelBuffer& target) : target_(target), clip_(target.width(), target.height()) {}

void Renderer::setClip(const Rect& r) noexcept { clip_ = r & Rect(target_.width(), target_.height()); }
void Renderer::resetClip() noexcept { clip_ = Rect(target_.width(), target_.height()); }

void Renderer::plot(int x, int y, Shade s) noexcept {
    if (s != Shade::Clear && clip_.contains({x, y})) target_(x, y) = s;
}

void Renderer::span(int x0, int x1, int y, Shade s) noexcept {
    if (y < clip_.y || y >= clip_.bottom()) return;
    x0 = std::max(x0, clip_.x);
    x1 = std::min(x1, clip_.right() - 1);
    for (int x = x0; x <= x1; ++x) target_(x, y) = s;
}

void Renderer::clear(Shade s) { fill(Rect(target_.width(), target_.height()) , s); }

void Renderer::pixel(Vec2i p, Shade s) { plot(p.x - camera_.x, p.y - camera_.y, s); }

void Renderer::line(Vec2i a, Vec2i b, Shade s) {
    a -= camera_;
    b -= camera_;
    // Bresenham: integer-only, so lines are identical on every platform.
    const int dx = std::abs(b.x - a.x), sx = a.x < b.x ? 1 : -1;
    const int dy = -std::abs(b.y - a.y), sy = a.y < b.y ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        plot(a.x, a.y, s);
        if (a == b) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; a.x += sx; }
        if (e2 <= dx) { err += dx; a.y += sy; }
    }
}

void Renderer::rect(const Rect& r, Shade s) {
    if (r.empty()) return;
    const int x0 = r.x - camera_.x, y0 = r.y - camera_.y, x1 = x0 + r.w - 1, y1 = y0 + r.h - 1;
    span(x0, x1, y0, s);
    span(x0, x1, y1, s);
    for (int y = y0 + 1; y < y1; ++y) { plot(x0, y, s); plot(x1, y, s); }
}

void Renderer::fill(const Rect& r, Shade s) {
    if (s == Shade::Clear) return;
    const int x0 = r.x - camera_.x, y0 = r.y - camera_.y;
    for (int y = y0; y < y0 + r.h; ++y) span(x0, x0 + r.w - 1, y, s);
}

void Renderer::fill(const Rect& r, Dither d, Shade a, Shade b) {
    const Rect area = Rect(r.x - camera_.x, r.y - camera_.y, r.w, r.h) & clip_;
    const int level = static_cast<int>(d);
    for (int y = area.y; y < area.bottom(); ++y)
        for (int x = area.x; x < area.right(); ++x) {
            const Shade s = kBayer[y & 3][x & 3] < level ? b : a;
            if (s != Shade::Clear) target_(x, y) = s;
        }
}

void Renderer::circle(Vec2i c, int r, Shade s) {
    c -= camera_;
    // Midpoint circle: eight-way symmetry from one octant.
    int x = r, y = 0, err = 1 - r;
    while (x >= y) {
        plot(c.x + x, c.y + y, s); plot(c.x + y, c.y + x, s);
        plot(c.x - y, c.y + x, s); plot(c.x - x, c.y + y, s);
        plot(c.x - x, c.y - y, s); plot(c.x - y, c.y - x, s);
        plot(c.x + y, c.y - x, s); plot(c.x + x, c.y - y, s);
        ++y;
        if (err < 0) err += 2 * y + 1;
        else { --x; err += 2 * (y - x) + 1; }
    }
}

void Renderer::fillCircle(Vec2i c, int r, Shade s) {
    c -= camera_;
    for (int dy = -r; dy <= r; ++dy) {
        int dx = 0;
        while ((dx + 1) * (dx + 1) + dy * dy <= r * r + r) ++dx;   // +r matches the outline's extent
        span(c.x - dx, c.x + dx, c.y + dy, s);
    }
}

void Renderer::draw(const Sprite& s, Vec2i at, Flip f) {
    at -= camera_;
    for (int y = 0; y < s.height(); ++y)
        for (int x = 0; x < s.width(); ++x) {
            const int sx = f == Flip::Horizontal ? s.width() - 1 - x : x;
            const int sy = f == Flip::Vertical ? s.height() - 1 - y : y;
            plot(at.x + x, at.y + y, s(sx, sy));
        }
}

void Renderer::draw(const SpriteSheet& sheet, int frame, Vec2i at, Flip f) { draw(sheet.frame(frame), at, f); }

int Renderer::text(std::string_view str, Vec2i at, const TextStyle& style) {
    const BitmapFont& f = font(style.font());
    const int w = f.textWidth(str);
    if (style.align() == Align::Center) at.x -= w / 2;
    else if (style.align() == Align::Right) at.x -= w;
    const Shade ink = style.inverted() ? invert(style.shade()) : style.shade();
    if (style.inverted()) fill(Rect(at.x - 1, at.y - 1, w + 2, f.glyphHeight() + 2), style.shade());
    at -= camera_;
    for (std::size_t i = 0; i < str.size(); ++i) {
        const int gx = at.x + static_cast<int>(i) * f.advance();
        for (int y = 0; y < f.glyphHeight(); ++y)
            for (int x = 0; x < f.glyphWidth(); ++x)
                if (f.pixel(str[i], x, y)) plot(gx + x, at.y + y, ink);
    }
    return w;
}

} // namespace nb::gfx
