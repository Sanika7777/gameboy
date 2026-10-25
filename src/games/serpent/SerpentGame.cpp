#include "games/serpent/SerpentGame.hpp"

#include "io/Paths.hpp"
#include "io/TextReader.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace nb::games::serpent {

using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

namespace {

constexpr Vec2i delta(Direction d) noexcept {
    switch (d) {
    case Direction::Up: return {0, -1};
    case Direction::Down: return {0, 1};
    case Direction::Left: return {-1, 0};
    case Direction::Right: return {1, 0};
    }
    return {};
}

constexpr bool opposite(Direction a, Direction b) noexcept { return delta(a) + delta(b) == Vec2i{0, 0}; }

const char* headName(Direction d, bool open) {
    static constexpr const char* kNames[2][4] = {{"head_up", "head_down", "head_left", "head_right"},
                                                 {"open_up", "open_down", "open_left", "open_right"}};
    return kNames[open ? 1 : 0][static_cast<int>(d)];
}

} // namespace

std::vector<Maze> loadMazes(const std::filesystem::path& file) {
    io::TextReader r(file);
    std::vector<Maze> out;
    while (r.next()) {
        const auto t = r.tokens();
        if (t.size() != 2 || t[0] != "maze") r.fail("expected: maze <name>");
        Maze m{std::string(t[1]), Grid<std::uint8_t>(SerpentGame::kCells.x, SerpentGame::kCells.y, 0), {-1, -1}};
        for (int y = 0; y < SerpentGame::kCells.y; ++y) {
            if (!r.nextRaw()) r.fail("unexpected end of file in maze '" + m.name + "'");
            const auto row = r.line();
            if (static_cast<int>(row.size()) != SerpentGame::kCells.x) r.fail("maze rows must be 40 characters");
            for (int x = 0; x < SerpentGame::kCells.x; ++x) {
                const char c = row[static_cast<std::size_t>(x)];
                if (c == '#') m.walls(x, y) = 1;
                else if (c == 'S') m.start = {x, y};
                else if (c != '.') r.fail(std::string("bad maze cell '") + c + "'");
            }
        }
        if (m.start.x < 0) r.fail("maze '" + m.name + "' has no S start cell");
        if (!r.next() || r.line() != "end") r.fail("missing 'end'");
        out.push_back(std::move(m));
    }
    return out;
}

const GameInfo& SerpentGame::staticInfo() {
    static const GameInfo info{"serpent", "SERPENT", "EAT, GROW, DON'T BITE", io::ScoreOrder::HigherIsBetter};
    return info;
}

SerpentGame::SerpentGame(GameContext& ctx)
    : GridGame(ctx, kCells, 4, {0, 8}),
      sprites_(ctx.assets.sprites("sprites/serpent.spr")),
      mazes_(loadMazes(io::Paths::asset("levels/serpent_mazes.txt"))) {
    occupied_.reserve(static_cast<std::size_t>(kCells.x * kCells.y));
}

std::string_view SerpentGame::modeName(int m) const {
    if (m == 0) return "BOX";
    if (m == 1) return "WRAP";
    return mazes_[static_cast<std::size_t>(m - 2)].name;
}

std::string SerpentGame::scoreTableKey() const { return mode_ == 0 ? "serpent.box" : mode_ == 1 ? "serpent.wrap" : "serpent.maze"; }

void SerpentGame::reset() {
    state_ = State::Setup;
    score_ = 0;
}

void SerpentGame::begin(int mode, int level) {
    mode_ = std::clamp(mode, 0, modeCount() - 1);
    level_ = std::clamp(level, 1, 9);
    body_.clear();
    occupied_.clear();
    turns_ = {};
    const Vec2i start = mode_ >= 2 ? mazes_[static_cast<std::size_t>(mode_ - 2)].start : Vec2i{10, 17};
    dir_ = Direction::Right;
    for (int i = 0; i < 4; ++i) {
        body_.push_back(start - Vec2i{i, 0});
        occupied_.insert(body_.back());
    }
    score_ = 0;
    eaten_ = grow_ = dyingFrames_ = 0;
    critter_ = {-1, -1};
    critterLeft_ = Seconds{0};
    stepTimer_ = Seconds{0};
    state_ = State::Playing;
    placeFood();
}

Vec2i SerpentGame::wrap(Vec2i c) const noexcept { return {(c.x + kCells.x) % kCells.x, (c.y + kCells.y) % kCells.y}; }

bool SerpentGame::blocked(Vec2i c) const {
    if (mode_ >= 2 && mazes_[static_cast<std::size_t>(mode_ - 2)].walls(c)) return true;
    return occupied_.contains(c);
}

void SerpentGame::placeFood() {
    // Random probes first (fast while the board is empty), then a deterministic scan.
    auto free = [&](Vec2i c) { return !blocked(c) && c != critter_; };
    for (int tries = 0; tries < 200; ++tries) {
        const Vec2i c{ctx_.rng.range(0, kCells.x - 1), ctx_.rng.range(0, kCells.y - 1)};
        if (free(c)) { food_ = c; return; }
    }
    for (int y = 0; y < kCells.y; ++y)
        for (int x = 0; x < kCells.x; ++x)
            if (free({x, y})) { food_ = {x, y}; return; }
    food_ = {-1, -1};   // the board is full: nothing left to eat
}

void SerpentGame::queueTurn(Direction d) {
    if (turns_.size() >= kMaxTurns) return;
    const Direction last = turns_.empty() ? dir_ : turns_.back();
    if (d == last || opposite(d, last)) return;   // no 180° reversals, no duplicates
    turns_.push(d);
}

void SerpentGame::handleInput(const input::InputState& in) {
    ++frames_;
    if (state_ == State::Setup) {
        if (in.pressed(Button::Up) || in.pressed(Button::Down)) { setupRow_ ^= 1; sfx("move"); }
        const int dx = in.pressed(Button::Right) ? 1 : in.pressed(Button::Left) ? -1 : 0;
        if (dx != 0) {
            if (setupRow_ == 0) mode_ = (mode_ + dx + modeCount()) % modeCount();
            else level_ = std::clamp(level_ + dx, 1, 9);
            sfx("move");
        }
        if (in.pressed(Button::A) || in.pressed(Button::Start) || in.lcdTap()) {
            sfx("select");
            begin(mode_, level_);
        }
        return;
    }
    if (state_ != State::Playing) return;
    if (in.pressed(Button::Up)) queueTurn(Direction::Up);
    if (in.pressed(Button::Down)) queueTurn(Direction::Down);
    if (in.pressed(Button::Left)) queueTurn(Direction::Left);
    if (in.pressed(Button::Right)) queueTurn(Direction::Right);
    if (const auto g = in.gesture()) {
        using K = input::Gesture::Kind;
        if (g->kind == K::SwipeUp) queueTurn(Direction::Up);
        else if (g->kind == K::SwipeDown) queueTurn(Direction::Down);
        else if (g->kind == K::SwipeLeft) queueTurn(Direction::Left);
        else if (g->kind == K::SwipeRight) queueTurn(Direction::Right);
    }
}

void SerpentGame::updateWorld(Seconds dt) {
    if (state_ == State::Dying) {
        if (++dyingFrames_ > 60) state_ = State::Over;
        return;
    }
    if (state_ != State::Playing) return;
    if (critter_.x >= 0) {
        critterLeft_ -= dt;
        if (critterLeft_ <= Seconds{0}) critter_ = {-1, -1};
    }
    stepTimer_ += dt;
    if (stepTimer_ >= stepInterval()) {
        stepTimer_ -= stepInterval();
        advance();
    }
}

void SerpentGame::advance() {
    if (!turns_.empty()) {
        dir_ = turns_.front();
        turns_.pop();
    }
    Vec2i next = body_.front() + delta(dir_);
    const bool outside = next.x < 0 || next.y < 0 || next.x >= kCells.x || next.y >= kCells.y;
    if (outside) {
        if (!wraps()) {
            state_ = State::Dying;
            sfx("die");
            buzz(40);
            return;
        }
        next = wrap(next);
    }
    // The tail moves out of the way this step unless we're growing.
    const Vec2i tail = body_.back();
    const bool tailMoves = grow_ == 0;
    if (blocked(next) && !(tailMoves && next == tail)) {
        state_ = State::Dying;
        sfx("die");
        buzz(40);
        return;
    }
    if (tailMoves) {
        occupied_.erase(tail);
        body_.pop_back();
    } else {
        --grow_;
    }
    body_.push_front(next);
    occupied_.insert(next);

    if (next == food_) {
        score_ += static_cast<std::uint32_t>(level_);
        grow_ += 2;
        ++eaten_;
        sfx("eat");
        if (eaten_ % 5 == 0) {
            critter_ = {-1, -1};
            for (int tries = 0; tries < 200 && critter_.x < 0; ++tries) {
                const Vec2i c{ctx_.rng.range(0, kCells.x - 1), ctx_.rng.range(0, kCells.y - 1)};
                if (!blocked(c) && c != food_ && c != next) critter_ = c;
            }
            critterLeft_ = Seconds{5.0};
            if (critter_.x >= 0) sfx("critter");
        }
        placeFood();
    } else if (next == critter_) {
        const int secondsLeft = static_cast<int>(std::ceil(critterLeft_.count()));
        score_ += static_cast<std::uint32_t>(level_ * (secondsLeft + 5));
        critter_ = {-1, -1};
        sfx("bonus");
    }
}

bool SerpentGame::foodAhead() const noexcept {
    Vec2i p = body_.front();
    for (int i = 0; i < 2; ++i) {
        p = wrap(p + delta(dir_));
        if (p == food_ || p == critter_) return true;
    }
    return false;
}

void SerpentGame::drawWorld(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    if (state_ == State::Setup) {
        r.fill(Rect(0, 0, 160, 144), gfx::Dither::Quarter, Shade::Paper, Shade::Light);
        r.fill(Rect(20, 20, 120, 100), Shade::Paper);
        r.rect(Rect(20, 20, 120, 100), Shade::Ink);
        r.text("SERPENT", {80, 28}, TextStyle{}.align(Align::Center));
        char modeLine[32];
        const auto name = modeName(mode_);
        std::snprintf(modeLine, sizeof modeLine, "MODE  <%.*s>", static_cast<int>(name.size()), name.data());
        r.text(modeLine, {30, 52}, TextStyle{}.inverted(setupRow_ == 0));
        const char speed[] = {'S', 'P', 'E', 'E', 'D', ' ', '<', static_cast<char>('0' + level_), '>', '\0'};
        r.text(speed, {30, 68}, TextStyle{}.inverted(setupRow_ == 1));
        const auto mini = TextStyle{}.font(gfx::Font::Mini).shade(Shade::Dark).align(Align::Center);
        r.text("UP/DOWN: PICK  L/R: CHANGE", {80, 92}, mini);
        r.text("A: GO", {80, 102}, mini);
        return;
    }
    if (mode_ >= 2) {
        const auto& walls = mazes_[static_cast<std::size_t>(mode_ - 2)].walls;
        for (int y = 0; y < kCells.y; ++y)
            for (int x = 0; x < kCells.x; ++x)
                if (walls(x, y)) r.fill(cellRect({x, y}), gfx::Dither::Half, Shade::Dark, Shade::Ink);
    }
    if (!wraps()) r.rect(Rect(0, 8, 160, 136), Shade::Ink);

    if (food_.x >= 0) r.draw(sprites_->get("food"), cellToScreen(food_));
    if (critter_.x >= 0) r.draw(*sprites_, sprites_->indexOf("critter") + (frames_ / 8) % 2, cellToScreen(critter_));

    // Hide the snake on alternate frames while dying: a classic blink.
    if (state_ == State::Dying && !ctx_.settings.reduceFlashing && (dyingFrames_ / 6) % 2) return;
    for (std::size_t i = 1; i < body_.size(); ++i) {
        const Rect c = cellRect(body_[i]);
        r.fill(Rect(c.x, c.y, 3, 3), i + 1 == body_.size() ? Shade::Dark : Shade::Ink);
    }
    r.draw(sprites_->get(headName(dir_, foodAhead() && state_ == State::Playing)), cellToScreen(body_.front()));
}

void SerpentGame::drawHud(gfx::Renderer& r) const {
    if (state_ == State::Setup) return;
    r.fill(Rect(0, 0, 160, 8), Shade::Ink);
    char buf[24];
    std::snprintf(buf, sizeof buf, "%06u", static_cast<unsigned>(score_));
    r.text(buf, {2, 2}, TextStyle{}.font(gfx::Font::Mini).shade(Shade::Paper));
    std::snprintf(buf, sizeof buf, "LV%d", level_);
    r.text(buf, {158, 2}, TextStyle{}.font(gfx::Font::Mini).shade(Shade::Paper).align(Align::Right));
    if (critter_.x >= 0) {
        const int w = static_cast<int>(critterLeft_.count() / 5.0 * 60);
        r.fill(Rect(50, 2, 60, 4), Shade::Dark);
        r.fill(Rect(50, 2, w, 4), Shade::Paper);
    }
}

void SerpentGame::save(io::BinaryWriter& w) const {
    w << static_cast<std::uint8_t>(state_) << mode_ << level_ << static_cast<std::uint8_t>(dir_);
    w << static_cast<std::uint32_t>(body_.size());
    for (const auto& c : body_) w << c.x << c.y;
    auto copy = turns_;
    w << static_cast<std::uint8_t>(copy.size());
    for (; !copy.empty(); copy.pop()) w << static_cast<std::uint8_t>(copy.front());
    w << food_.x << food_.y << critter_.x << critter_.y << critterLeft_.count() << stepTimer_.count();
    w << eaten_ << grow_ << score_ << ctx_.rng.state();
}

void SerpentGame::load(io::BinaryReader& r) {
    state_ = static_cast<State>(r.get<std::uint8_t>());
    mode_ = std::clamp(r.get<int>(), 0, modeCount() - 1);
    level_ = std::clamp(r.get<int>(), 1, 9);
    dir_ = static_cast<Direction>(r.get<std::uint8_t>() & 3);
    const auto n = r.get<std::uint32_t>();
    if (n == 0 || n > static_cast<std::uint32_t>(kCells.x * kCells.y)) throw CorruptSaveError("SAVE CORRUPT", "snake length");
    body_.clear();
    occupied_.clear();
    for (std::uint32_t i = 0; i < n; ++i) {
        Vec2i c;
        r >> c.x >> c.y;
        body_.push_back(c);
        occupied_.insert(c);
    }
    turns_ = {};
    for (auto k = r.get<std::uint8_t>(); k > 0; --k) turns_.push(static_cast<Direction>(r.get<std::uint8_t>() & 3));
    r >> food_.x >> food_.y >> critter_.x >> critter_.y;
    critterLeft_ = Seconds{r.get<double>()};
    stepTimer_ = Seconds{r.get<double>()};
    r >> eaten_ >> grow_ >> score_;
    ctx_.rng.setState(r.get<std::string>());
}

} // namespace nb::games::serpent
