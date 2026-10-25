#include "games/stackfall/StackfallGame.hpp"

#include "io/Paths.hpp"
#include "io/TextReader.hpp"

#include <algorithm>
#include <cstdio>

namespace nb::games::stackfall {
using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

namespace {
constexpr Vec2i kOrigin{4, 2};
constexpr int kCell = 7;

const Cells& shapeOf(Kind k, int rot) {
    return kShapes[static_cast<std::size_t>(k)][static_cast<std::size_t>(rot & 3)];
}

}

Tuning Tuning::load(const std::filesystem::path& file) {
    io::TextReader r(file);
    Tuning t;
    while (r.next()) {
        const auto tok = r.tokens();
        if (tok[0] == "gravity") {
            for (std::size_t i = 1; i < tok.size(); ++i) t.gravity.push_back(io::parseInt(r, tok[i]));
            if (t.gravity.empty() || std::any_of(t.gravity.begin(), t.gravity.end(), [](int g) { return g < 1; }))
                r.fail("gravity needs at least one value ≥ 1");
            continue;
        }
        if (tok.size() != 2) r.fail("expected: <name> <value>");
        const int v = io::parseInt(r, tok[1]);
        if (v < 0) r.fail("negative value");
        if (tok[0] == "das") t.das = v;
        else if (tok[0] == "arr") t.arr = v;
        else if (tok[0] == "lock") t.lockDelay = v;
        else if (tok[0] == "lock_resets") t.lockResets = v;
        else r.fail("unknown setting '" + std::string(tok[0]) + "'");
    }
    if (t.gravity.empty()) r.fail("missing gravity");
    return t;
}

const GameInfo& StackfallGame::staticInfo() {
    static const GameInfo info{"stackfall", "STACKFALL", "CLEAR LINES, STAY LOW", io::ScoreOrder::HigherIsBetter};
    return info;
}

StackfallGame::StackfallGame(GameContext& ctx)
    : GridGame(ctx, {kWidth, kHeight - kHidden}, kCell, kOrigin),
      tuning_(Tuning::load(io::Paths::asset("data/stackfall.txt"))) {
    clearing_.reserve(4);
    reset();
}

std::uint32_t StackfallGame::lineScore(int lines, int level) noexcept {
    static constexpr std::array<std::uint32_t, 5> kBase{0, 100, 300, 500, 800};
    return kBase[static_cast<std::size_t>(std::clamp(lines, 0, 4))] * static_cast<std::uint32_t>(level);
}

void StackfallGame::reset() {
    board_ = Grid<std::uint8_t>(kWidth, kHeight, 0);
    bagPos_ = kKinds;
    next_.clear();
    while (next_.size() < 3) next_.push_back(nextFromBag());
    hold_.reset();
    canHold_ = true;
    lines_ = 0;
    level_ = 1;
    score_ = 0;
    clearing_.clear();
    state_ = State::Falling;
    dasDir_ = 0;
    spawn(nextFromBag());
}

Kind StackfallGame::nextFromBag() {
    if (bagPos_ >= kKinds) {
        for (int i = 0; i < kKinds; ++i) bag_[static_cast<std::size_t>(i)] = static_cast<Kind>(i);
        std::shuffle(bag_.begin(), bag_.end(), ctx_.rng.engine());
        bagPos_ = 0;
    }
    return bag_[static_cast<std::size_t>(bagPos_++)];
}

bool StackfallGame::fits(const Piece& p) const {
    return std::all_of(shapeOf(p.kind, p.rot).begin(), shapeOf(p.kind, p.rot).end(), [&](Vec2i c) {
        const Vec2i q = p.pos + c;
        return board_.contains(q) && board_(q) == 0;
    });
}

void StackfallGame::spawn(Kind k) {
    cur_ = Piece{k, 0, {k == Kind::O ? 4 : 3, kHidden - 1}};
    fallTimer_ = lockTimer_ = resets_ = 0;
    canHold_ = true;
    if (!fits(cur_)) {
        state_ = State::Dying;
        stateFrames_ = 0;
        sfx("gameover");
        buzz(40);
    }
}

bool StackfallGame::shift(int dx) {
    Piece p = cur_;
    p.pos.x += dx;
    if (!fits(p)) return false;
    cur_ = p;
    touchedAfterMove();
    return true;
}

bool StackfallGame::rotate(int dir) {
    if (cur_.kind == Kind::O) return false;
    Piece p = cur_;
    p.rot = (p.rot + dir + 4) & 3;
    const auto& kicks = cur_.kind == Kind::I ? kKicksI : kKicks;
    for (const Vec2i k : kicks) {
        Piece q = p;
        q.pos.x += dir > 0 ? k.x : -k.x;
        q.pos.y += k.y;
        if (fits(q)) {
            cur_ = q;
            touchedAfterMove();
            return true;
        }
    }
    return false;
}

void StackfallGame::touchedAfterMove() {
    Piece below = cur_;
    ++below.pos.y;
    if (!fits(below) && resets_ < tuning_.lockResets) {
        lockTimer_ = 0;
        ++resets_;
    }
}

StackfallGame::Piece StackfallGame::ghost() const {
    Piece g = cur_;
    for (Piece n = g; ++n.pos.y, fits(n);) g = n;
    return g;
}

int StackfallGame::gravityFrames() const {
    const auto& g = tuning_.gravity;
    return g[static_cast<std::size_t>(std::min<int>(level_ - 1, static_cast<int>(g.size()) - 1))];
}

void StackfallGame::lock() {
    for (const Vec2i c : shapeOf(cur_.kind, cur_.rot)) board_(cur_.pos + c) = static_cast<std::uint8_t>(1 + static_cast<int>(cur_.kind));
    clearing_.clear();
    for (int y = 0; y < kHeight; ++y) {
        const auto rowBegin = board_.begin() + y * kWidth;
        if (std::all_of(rowBegin, rowBegin + kWidth, [](std::uint8_t v) { return v != 0; })) clearing_.push_back(y);
    }
    if (!clearing_.empty()) {
        state_ = State::Clearing;
        stateFrames_ = 0;
        sfx("line");
        buzz(clearing_.size() == 4 ? 40 : 20);
        return;
    }
    sfx("lock");
    spawn(next_.front());
    next_.pop_front();
    next_.push_back(nextFromBag());
}

void StackfallGame::collapse() {
    int write = kHeight - 1;
    for (int y = kHeight - 1; y >= 0; --y) {
        if (std::find(clearing_.begin(), clearing_.end(), y) != clearing_.end()) continue;
        if (write != y)
            for (int x = 0; x < kWidth; ++x) board_(x, write) = board_(x, y);
        --write;
    }
    for (; write >= 0; --write)
        for (int x = 0; x < kWidth; ++x) board_(x, write) = 0;
    const int n = static_cast<int>(clearing_.size());
    score_ += lineScore(n, level_);
    lines_ += n;
    level_ = 1 + lines_ / 10;
    clearing_.clear();
    state_ = State::Falling;
    spawn(next_.front());
    next_.pop_front();
    next_.push_back(nextFromBag());
}

void StackfallGame::handleInput(const input::InputState& in) {
    if (state_ != State::Falling) return;
    if (in.pressed(Button::A) && rotate(+1)) sfx("rotate");
    if (in.pressed(Button::B) && rotate(-1)) sfx("rotate");
    if (in.pressed(Button::Select) && canHold_) {
        const Kind k = cur_.kind;
        if (hold_) spawn(*hold_);
        else {
            spawn(next_.front());
            next_.pop_front();
            next_.push_back(nextFromBag());
        }
        hold_ = k;
        canHold_ = false;
        sfx("hold");
        return;
    }
    if (in.pressed(Button::Up)) {
        const Piece g = ghost();
        score_ += static_cast<std::uint32_t>(2 * (g.pos.y - cur_.pos.y));
        cur_ = g;
        sfx("drop");
        lock();
        return;
    }
    const int dir = in.held(Button::Right) - in.held(Button::Left);
    if (dir != dasDir_) {
        dasDir_ = dir;
        dasTimer_ = 0;
        if (dir != 0 && shift(dir)) sfx("shift");
    } else if (dir != 0 && ++dasTimer_ >= tuning_.das) {
        if (tuning_.arr == 0) {
            while (shift(dir)) {}
        } else if ((dasTimer_ - tuning_.das) % tuning_.arr == 0) {
            shift(dir);
        }
    }
    if (in.held(Button::Down)) {
        Piece p = cur_;
        ++p.pos.y;
        if (fits(p)) {
            cur_ = p;
            fallTimer_ = 0;
            ++score_;
        }
    }
}

void StackfallGame::updateWorld(double) {
    ++stateFrames_;
    if (state_ == State::Clearing) {
        if (stateFrames_ >= 18) collapse();
        return;
    }
    if (state_ == State::Dying) {
        if (stateFrames_ > 60) state_ = State::Over;
        return;
    }
    if (state_ != State::Falling) return;
    Piece below = cur_;
    ++below.pos.y;
    if (fits(below)) {
        lockTimer_ = 0;
        if (++fallTimer_ >= gravityFrames()) {
            fallTimer_ = 0;
            cur_ = below;
        }
    } else if (++lockTimer_ >= tuning_.lockDelay) {
        lock();
    }
}

void StackfallGame::drawBlock(gfx::Renderer& r, Vec2i at, Kind k, int px) const {
    const Rect b(at.x, at.y, px - 1, px - 1);
    switch (k) {
    case Kind::I: r.fill(b, Shade::Light); break;
    case Kind::O: r.fill(b, Shade::Dark); break;
    case Kind::T: r.fill(b, Shade::Paper); break;
    case Kind::S: r.fill(b, gfx::Dither::Half, Shade::Light, Shade::Dark); break;
    case Kind::Z: r.fill(b, gfx::Dither::Quarter, Shade::Paper, Shade::Ink); break;
    case Kind::J: r.fill(b, Shade::Ink); break;
    case Kind::L: r.fill(b, gfx::Dither::ThreeQuarter, Shade::Light, Shade::Ink); break;
    }
    r.rect(b, Shade::Ink);
    if (px > 4 && k == Kind::J) r.pixel({b.x + 2, b.y + 2}, Shade::Light);
}

void StackfallGame::drawWorld(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    r.fill(Rect(76, 0, 84, 144), gfx::Dither::Quarter, Shade::Paper, Shade::Light);
    drawFrame(r);
    const bool flashOn = ctx_.settings.reduceFlashing || (stateFrames_ / 3) % 2 == 0;
    for (int y = kHidden; y < kHeight; ++y) {
        const bool clearing = state_ == State::Clearing && std::find(clearing_.begin(), clearing_.end(), y) != clearing_.end();
        for (int x = 0; x < kWidth; ++x) {
            const auto v = board_(x, y);
            if (v == 0) continue;
            const Vec2i at = cellToScreen({x, y - kHidden});
            if (clearing) r.fill(Rect(at.x, at.y, kCell, kCell), flashOn ? (ctx_.settings.reduceFlashing ? Shade::Dark : Shade::Ink) : Shade::Paper);
            else drawBlock(r, at, static_cast<Kind>(v - 1), kCell);
        }
    }
    if (state_ != State::Falling) {
        if (state_ == State::Dying || state_ == State::Over)
            r.fill(Rect(kOrigin.x, kOrigin.y, kWidth * kCell, (kHeight - kHidden) * kCell), gfx::Dither::Half, Shade::Clear, Shade::Dark);
        return;
    }
    const Piece g = ghost();
    for (const Vec2i c : shapeOf(g.kind, g.rot)) {
        const Vec2i cell = g.pos + c;
        if (cell.y < kHidden) continue;
        const Vec2i at = cellToScreen({cell.x, cell.y - kHidden});
        r.rect(Rect(at.x, at.y, kCell - 1, kCell - 1), Shade::Light);
    }
    for (const Vec2i c : shapeOf(cur_.kind, cur_.rot)) {
        const Vec2i cell = cur_.pos + c;
        if (cell.y >= kHidden) drawBlock(r, cellToScreen({cell.x, cell.y - kHidden}), cur_.kind, kCell);
    }
}

void StackfallGame::drawHud(gfx::Renderer& r) const {
    const auto mini = TextStyle{}.font(gfx::Font::Mini);
    auto box = [&](Rect b, const char* label) {
        r.fill(b, Shade::Paper);
        r.rect(b, Shade::Ink);
        r.text(label, {b.x + 2, b.y + 2}, mini);
    };
    auto mini4 = [&](Kind k, Vec2i at) {
        for (const Vec2i c : shapeOf(k, 0)) drawBlock(r, at + c * 4, k, 4);
    };
    box(Rect(80, 4, 36, 30), "HOLD");
    if (hold_) mini4(*hold_, {88, 16});
    box(Rect(120, 4, 36, 66), "NEXT");
    for (int i = 0; i < 3; ++i) mini4(next_[static_cast<std::size_t>(i)], {128, 14 + i * 18});

    char buf[16];
    box(Rect(80, 76, 76, 62), "SCORE");
    std::snprintf(buf, sizeof buf, "%07u", static_cast<unsigned>(score_));
    r.text(buf, {84, 86});
    r.text("LEVEL", {84, 100}, mini);
    std::snprintf(buf, sizeof buf, "%d", level_);
    r.text(buf, {84, 108});
    r.text("LINES", {120, 100}, mini);
    std::snprintf(buf, sizeof buf, "%d", lines_);
    r.text(buf, {120, 108});
    r.text("SEL:HOLD  UP:DROP", {84, 124}, mini);
}

void StackfallGame::save(io::BinaryWriter& w) const {
    w << board_.raw() << bag_ << bagPos_;
    w << static_cast<std::uint8_t>(next_.size());
    for (const Kind k : next_) w << k;
    w << cur_.kind << cur_.rot << cur_.pos.x << cur_.pos.y << hold_.has_value() << hold_.value_or(Kind::I) << canHold_;
    w << fallTimer_ << lockTimer_ << resets_ << lines_ << level_ << score_ << ctx_.rng.state();
}

void StackfallGame::load(io::BinaryReader& r) {
    std::vector<std::uint8_t> raw;
    r >> raw;
    if (raw.size() != static_cast<std::size_t>(kWidth * kHeight) ||
        std::any_of(raw.begin(), raw.end(), [](std::uint8_t v) { return v > kKinds; }))
        throw CorruptSaveError("SAVE CORRUPT", "board");
    board_.raw() = std::move(raw);
    r >> bag_ >> bagPos_;
    next_.clear();
    for (auto n = r.get<std::uint8_t>(); n > 0; --n) next_.push_back(static_cast<Kind>(r.get<std::uint8_t>() % kKinds));
    if (next_.size() < 3) throw CorruptSaveError("SAVE CORRUPT", "queue");
    cur_.kind = static_cast<Kind>(r.get<std::uint8_t>() % kKinds);
    r >> cur_.rot >> cur_.pos.x >> cur_.pos.y;
    const bool held = r.get<bool>();
    const Kind hk = static_cast<Kind>(r.get<std::uint8_t>() % kKinds);
    hold_ = held ? std::optional{hk} : std::nullopt;
    r >> canHold_ >> fallTimer_ >> lockTimer_ >> resets_ >> lines_ >> level_ >> score_;
    ctx_.rng.setState(r.get<std::string>());
    bagPos_ = std::clamp(bagPos_, 0, kKinds);
    if (!fits(cur_)) throw CorruptSaveError("SAVE CORRUPT", "piece");
    state_ = State::Falling;
    clearing_.clear();
    dasDir_ = 0;
}

}
