#include "games/minefield/MinefieldGame.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <numeric>
#include <queue>

namespace nb::games::minefield {
using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

const GameInfo& MinefieldGame::staticInfo() {
    static const GameInfo info{"minefield", "MINEFIELD", "CLEAR THE FIELD, FAST", io::ScoreOrder::LowerIsBetter};
    return info;
}

MinefieldGame::MinefieldGame(GameContext& ctx)
    : GridGame(ctx, {9, 9}, 8, {44, 44}), sprites_(gfx::SpriteSheet::load(io::Paths::asset("sprites/minefield.spr"))) {
    mines_.reserve(99);
}

std::optional<std::uint32_t> MinefieldGame::result() const {
    return state_ == State::Over && boom_.x < 0 ? std::optional{seconds()} : std::nullopt;
}

void MinefieldGame::reset() { state_ = State::Setup; }

void MinefieldGame::begin(Difficulty d) {
    diff_ = d;
    cells_ = Grid<Cell>(spec().size.x, spec().size.y);
    flags_.clear();
    mines_.clear();
    cursor_ = {spec().size.x / 2, spec().size.y / 2};
    boom_ = {-1, -1};
    revealed_ = endFrames_ = 0;
    elapsed_ = 0;
    flagMode_ = false;
    state_ = State::Ready;
    layout();
}

void MinefieldGame::layout() {
    const Vec2i size = spec().size;
    if (size.x * 8 <= 160) {
        setGeometry(size, 8, {(160 - size.x * 8) / 2, 16 + (128 - size.y * 8) / 2});
        return;
    }
    const int cx = cursor_.x * 8 - scroll_;
    if (cx < 16) scroll_ = cursor_.x * 8 - 16;
    if (cx > 160 - 24) scroll_ = cursor_.x * 8 - (160 - 24);
    scroll_ = std::clamp(scroll_, 0, size.x * 8 - 160);
    setGeometry(size, 8, {-scroll_, 16});
}

void MinefieldGame::placeMines(Vec2i safe) {
    std::vector<int> candidates(static_cast<std::size_t>(cells_.width() * cells_.height()));
    std::iota(candidates.begin(), candidates.end(), 0);
    std::erase_if(candidates, [&](int i) {
        const Vec2i p{i % cells_.width(), i / cells_.width()};
        return std::abs(p.x - safe.x) <= 1 && std::abs(p.y - safe.y) <= 1;
    });
    std::shuffle(candidates.begin(), candidates.end(), ctx_.rng.engine());
    for (int k = 0; k < spec().mines; ++k) {
        const int i = candidates[static_cast<std::size_t>(k)];
        const Vec2i p{i % cells_.width(), i / cells_.width()};
        cells_(p).mine = 1;
        mines_.push_back(p);
    }
    for (int y = 0; y < cells_.height(); ++y)
        for (int x = 0; x < cells_.width(); ++x) {
            const auto n = cells_.neighbours({x, y});
            cells_(x, y).adjacent = static_cast<std::uint8_t>(
                std::count_if(n.begin(), n.end(), [&](Vec2i q) { return cells_(q).mine != 0; }));
        }
}

void MinefieldGame::flood(Vec2i start) {
    std::queue<Vec2i> open;
    open.push(start);
    while (!open.empty()) {
        const Vec2i c = open.front();
        open.pop();
        Cell& cell = cells_(c);
        if (cell.revealed || flags_.contains(c)) continue;
        cell.revealed = 1;
        ++revealed_;
        if (cell.adjacent != 0) continue;
        for (const Vec2i n : cells_.neighbours(c))
            if (!cells_(n).revealed) open.push(n);
    }
}

void MinefieldGame::reveal(Vec2i c) {
    if (flags_.contains(c) || cells_(c).revealed) return;
    if (state_ == State::Ready) {
        placeMines(c);
        state_ = State::Playing;
    }
    if (cells_(c).mine) {
        boom_ = c;
        cells_(c).revealed = 1;
        state_ = State::Lost;
        sfx("explode");
        buzz(40);
        return;
    }
    flood(c);
    sfx("reveal");
    if (revealed_ == cells_.width() * cells_.height() - static_cast<int>(mines_.size())) {
        state_ = State::Won;
        for (const Vec2i m : mines_) flags_.insert(m);
        sfx("win");
    }
}

int MinefieldGame::flagsAround(Vec2i c) const {
    const auto n = cells_.neighbours(c);
    return static_cast<int>(std::count_if(n.begin(), n.end(), [&](Vec2i q) { return flags_.contains(q); }));
}

void MinefieldGame::chord(Vec2i c) {
    const Cell& cell = cells_(c);
    if (!cell.revealed || cell.adjacent == 0 || flagsAround(c) != cell.adjacent) return;
    for (const Vec2i n : cells_.neighbours(c))
        if (state_ == State::Playing) reveal(n);
}

void MinefieldGame::toggleFlag(Vec2i c) {
    if (cells_(c).revealed || state_ == State::Ready) return;
    if (!flags_.erase(c)) flags_.insert(c);
    sfx("flag");
    buzz(10);
}

void MinefieldGame::act(Vec2i c, bool flag) {
    if (state_ != State::Ready && state_ != State::Playing) return;
    if (flag) toggleFlag(c);
    else if (cells_(c).revealed) chord(c);
    else reveal(c);
}

void MinefieldGame::moveCursor(Vec2i d) {
    cursor_.x = std::clamp(cursor_.x + d.x, 0, cells_.width() - 1);
    cursor_.y = std::clamp(cursor_.y + d.y, 0, cells_.height() - 1);
    layout();
}

void MinefieldGame::handleInput(const input::InputState& in) {
    ++frames_;
    if (state_ == State::Setup) {
        auto d = static_cast<int>(diff_);
        if (in.pressed(Button::Up)) d = (d + 2) % 3;
        if (in.pressed(Button::Down)) d = (d + 1) % 3;
        if (d != static_cast<int>(diff_)) sfx("move");
        diff_ = static_cast<Difficulty>(d);
        if (in.pressed(Button::A) || in.pressed(Button::Start)) {
            sfx("select");
            begin(diff_);
        }
        return;
    }
    if (state_ != State::Ready && state_ != State::Playing) return;

    const Vec2i dir{in.held(Button::Right) - in.held(Button::Left), in.held(Button::Down) - in.held(Button::Up)};
    const bool fresh = in.pressed(Button::Left) || in.pressed(Button::Right) || in.pressed(Button::Up) || in.pressed(Button::Down);
    if (dir == Vec2i{0, 0}) heldFrames_ = 0;
    else if (fresh || (++heldFrames_ > 14 && heldFrames_ % 4 == 0)) moveCursor(dir);

    digging_ = in.held(Button::A);
    if (in.pressed(Button::A)) act(cursor_, false);
    if (in.pressed(Button::B)) act(cursor_, true);
    if (in.pressed(Button::Select)) {
        flagMode_ = !flagMode_;
        sfx("move");
    }
    if (const auto g = in.gesture()) {
        if (const auto c = screenToCell(g->at)) {
            cursor_ = *c;
            if (g->kind == input::Gesture::Kind::Tap) act(*c, flagMode_);
            else if (g->kind == input::Gesture::Kind::LongPress) act(*c, true);
        }
    }
}

void MinefieldGame::updateWorld(double dt) {
    if (state_ == State::Playing) elapsed_ += dt;
    if (state_ == State::Lost) {
        ++endFrames_;
        if (endFrames_ % 3 == 0)
            for (const Vec2i m : mines_)
                if (!cells_(m).revealed) {
                    cells_(m).revealed = 1;
                    break;
                }
        if (endFrames_ > static_cast<int>(mines_.size()) * 3 + 60) state_ = State::Over;
    } else if (state_ == State::Won && ++endFrames_ > 60) {
        state_ = State::Over;
    }
}

void MinefieldGame::drawWorld(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    if (state_ == State::Setup) {
        r.fill(Rect(0, 0, 160, 144), gfx::Dither::Quarter, Shade::Paper, Shade::Light);
        r.fill(Rect(24, 24, 112, 96), Shade::Paper);
        r.rect(Rect(24, 24, 112, 96), Shade::Ink);
        r.text("MINEFIELD", {80, 32}, TextStyle{}.align(Align::Center));
        for (std::size_t i = 0; i < kSpecs.size(); ++i)
            r.text(kSpecs[i].name, {80, 54 + static_cast<int>(i) * 14},
                   TextStyle{}.align(Align::Center).inverted(i == static_cast<std::size_t>(diff_)));
        r.text("A: START", {80, 104}, TextStyle{}.font(gfx::Font::Mini).shade(Shade::Dark).align(Align::Center));
        return;
    }
    r.setClip(Rect(0, 16, 160, 128));
    if (cells_.width() * 8 <= 160) drawFrame(r);
    const bool lost = state_ == State::Lost || (state_ == State::Over && boom_.x >= 0);
    for (int y = 0; y < cells_.height(); ++y)
        for (int x = 0; x < cells_.width(); ++x) {
            const Vec2i c{x, y};
            const Rect b = cellRect(c);
            if (b.right() < 0 || b.x >= 160) continue;
            const Cell& cell = cells_(c);
            if (!cell.revealed) {
                r.fill(b, Shade::Light);
                r.line({b.x, b.y}, {b.right() - 1, b.y}, Shade::Paper);
                r.line({b.x, b.y}, {b.x, b.bottom() - 1}, Shade::Paper);
                r.line({b.x, b.bottom() - 1}, {b.right() - 1, b.bottom() - 1}, Shade::Dark);
                r.line({b.right() - 1, b.y}, {b.right() - 1, b.bottom() - 1}, Shade::Dark);
                if (flags_.contains(c)) {
                    r.draw(sprites_.get("flag"), {b.x, b.y});
                    if (lost && !cell.mine) r.line({b.x + 1, b.y + 1}, {b.x + 6, b.y + 6}, Shade::Ink);
                }
                continue;
            }
            r.fill(b, c == boom_ ? Shade::Dark : Shade::Paper);
            r.line({b.x, b.bottom() - 1}, {b.right() - 1, b.bottom() - 1}, Shade::Light);
            r.line({b.right() - 1, b.y}, {b.right() - 1, b.bottom() - 1}, Shade::Light);
            if (cell.mine) {
                r.draw(sprites_.get("mine"), {b.x, b.y});
            } else if (cell.adjacent > 0) {
                const char digit[2] = {static_cast<char>('0' + cell.adjacent), '\0'};
                r.text(digit, {b.x + 2, b.y + 1}, TextStyle{}.font(gfx::Font::Mini).shade(cell.adjacent > 2 ? Shade::Ink : Shade::Dark));
            }
        }
    if ((state_ == State::Ready || state_ == State::Playing) && (frames_ / 15) % 2 == 0) {
        const Rect b = cellRect(cursor_);
        r.rect(Rect(b.x - 1, b.y - 1, b.w + 2, b.h + 2), Shade::Ink);
        r.rect(Rect(b.x, b.y, b.w, b.h), flagMode_ ? Shade::Dark : Shade::Ink);
    }
    r.resetClip();
}

void MinefieldGame::drawHud(gfx::Renderer& r) const {
    if (state_ == State::Setup) return;
    r.fill(Rect(0, 0, 160, 16), Shade::Light);
    r.line({0, 15}, {159, 15}, Shade::Ink);
    char buf[8];
    const int left = spec().mines - static_cast<int>(flags_.size());
    std::snprintf(buf, sizeof buf, "%03d", std::clamp(left, -99, 999));
    r.fill(Rect(3, 3, 21, 10), Shade::Ink);
    r.text(buf, {5, 4}, TextStyle{}.shade(Shade::Paper));
    std::snprintf(buf, sizeof buf, "%03u", static_cast<unsigned>(seconds()));
    r.fill(Rect(136, 3, 21, 10), Shade::Ink);
    r.text(buf, {138, 4}, TextStyle{}.shade(Shade::Paper));
    const char* face = state_ == State::Won || (state_ == State::Over && boom_.x < 0) ? "face_cool"
                     : boom_.x >= 0 ? "face_dead" : digging_ ? "face_wow" : "face_smile";
    r.draw(sprites_.get(face), {75, 2});
    r.text(flagMode_ ? "FLAG" : "DIG", {32, 6}, TextStyle{}.font(gfx::Font::Mini).shade(flagMode_ ? Shade::Ink : Shade::Dark));
}

void MinefieldGame::save(io::BinaryWriter& w) const {
    w << diff_ << state_ << cursor_.x << cursor_.y << elapsed_ << flagMode_ << static_cast<std::uint32_t>(revealed_);
    for (const Cell& c : cells_) w << c.mine << c.revealed << c.adjacent;
    w << static_cast<std::uint32_t>(flags_.size());
    for (const Vec2i f : flags_) w << f.x << f.y;
    w << ctx_.rng.state();
}

void MinefieldGame::load(io::BinaryReader& r) {
    const auto d = r.get<Difficulty>();
    if (static_cast<int>(d) > 2) throw CorruptSaveError("SAVE CORRUPT", "difficulty");
    begin(d);
    state_ = r.get<State>();
    if (state_ != State::Ready && state_ != State::Playing) throw CorruptSaveError("SAVE CORRUPT", "state");
    r >> cursor_.x >> cursor_.y >> elapsed_ >> flagMode_;
    revealed_ = static_cast<int>(r.get<std::uint32_t>());
    cursor_.x = std::clamp(cursor_.x, 0, cells_.width() - 1);
    cursor_.y = std::clamp(cursor_.y, 0, cells_.height() - 1);
    for (int y = 0; y < cells_.height(); ++y)
        for (int x = 0; x < cells_.width(); ++x) {
            Cell& c = cells_(x, y);
            r >> c.mine >> c.revealed >> c.adjacent;
            if (c.mine) mines_.push_back({x, y});
        }
    for (auto n = r.get<std::uint32_t>(); n > 0; --n) {
        Vec2i f;
        r >> f.x >> f.y;
        if (cells_.contains(f)) flags_.insert(f);
    }
    ctx_.rng.setState(r.get<std::string>());
    layout();
}

}
