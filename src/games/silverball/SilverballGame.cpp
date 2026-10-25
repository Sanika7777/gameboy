#include "games/silverball/SilverballGame.hpp"

#include "io/Paths.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <numeric>

namespace nb::games::silverball {
using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

const GameInfo& SilverballGame::staticInfo() {
    static const GameInfo info{"silverball", "SILVERBALL", "FLIP IT, NUDGE IT, DON'T TILT", io::ScoreOrder::HigherIsBetter};
    return info;
}

SilverballGame::SilverballGame(GameContext& ctx)
    : PhysicsGame(ctx, 4), table_(loadTable(io::Paths::asset("tables/silverball_classic.tbl"))) {
    auto ball = std::make_unique<Ball>();
    ball_ = ball.get();
    ball_->gravity = table_.gravity;
    objects_.push_back(std::move(ball));
    reset();
}

void SilverballGame::reset() {
    score_ = bonus_ = 0;
    balls_ = kBalls;
    multiplier_ = 1;
    elapsed_ = 0;
    tilted_ = false;
    timers_ = {};
    nudges_.clear();
    sparks_.clear();
    for (auto* t : table_.targets) t->raise();
    for (auto* r : table_.rollovers) r->unlight();
    newBall();
}

void SilverballGame::newBall() {
    ball_->pos = table_.plunger->restPosition();
    ball_->vel = {};
    state_ = State::Launch;
    tilted_ = false;
    targetsHit_ = rolloversHit_ = 0;
    ballSaveLit_ = true;
    ballSaveUntil_ = elapsed_ + 1e9;
    camera_.pos = {0, static_cast<float>(table_.size.y - 144)};
}

void SilverballGame::spark(Vec2f at) {
    for (int i = 0; i < 4; ++i)
        sparks_.push_back({at, {ctx_.rng.real(-60, 60), ctx_.rng.real(-80, 20)}, 12 + ctx_.rng.range(0, 6)});
}

void SilverballGame::targetDown(char group) {
    ++targetsHit_;
    const bool all = std::all_of(table_.targets.begin(), table_.targets.end(),
                                 [&](const DropTarget* t) { return t->group() != group || t->isDown(); });
    if (!all) return;
    multiplier_ = std::min(5, multiplier_ + 1);
    sfx("bonus");
    ctx_.events.toast.emit("MULTIPLIER UP!");
    for (auto* t : table_.targets)
        if (t->group() == group) t->raise();
}

void SilverballGame::rolloverLit() {
    ++rolloversHit_;
    const bool all = std::all_of(table_.rollovers.begin(), table_.rollovers.end(), [](const Rollover* r) { return r->lit(); });
    if (all) {
        award(1000);
        sfx("bonus");
        for (auto* r : table_.rollovers) r->unlight();
    } else {
        timers_.push({elapsed_ + 20.0, Timer::LightsOut});
    }
}

void SilverballGame::nudge() {
    nudges_.push_back(elapsed_);
    while (!nudges_.empty() && nudges_.front() < elapsed_ - 5.0) nudges_.pop_front();
    if (nudges_.size() >= 3 && !tilted_) {
        tilted_ = true;
        sfx("tilt");
        buzz(40);
        ctx_.events.toast.emit("TILT");
        return;
    }
    ball_->vel += Vec2f{ctx_.rng.real(-40, 40), -60};
    buzz(20);
}

void SilverballGame::handleInput(const input::InputState& in) {
    if (state_ == State::Over || state_ == State::Bonus) return;
    const auto touch = in.lcdTouch();
    const bool left = in.held(Button::Left) || in.held(Button::B) || (touch && touch->x < 80);
    const bool right = in.held(Button::A) || in.held(Button::Right) || (touch && touch->x >= 80);
    if (!tilted_) {
        if ((left && in.pressed(Button::Left)) || in.pressed(Button::B) || (in.pressed(Button::A) || in.pressed(Button::Right))) sfx("flipper");
        table_.leftFlipper->setPressed(left);
        table_.rightFlipper->setPressed(right);
    } else {
        table_.leftFlipper->setPressed(false);
        table_.rightFlipper->setPressed(false);
    }
    table_.plunger->setPulling(in.held(Button::Down) && state_ == State::Launch);
    if (in.pressed(Button::Select) && state_ == State::Play) nudge();
}

void SilverballGame::resolve(TableElement& e, const Contact& c) {
    ball_->pos += c.normal * c.depth;
    const Vec2f rel = ball_->vel - c.surfaceVel;
    const float vn = dot(rel, c.normal);
    if (vn < 0) ball_->vel = rel - c.normal * ((1.0f + e.restitution()) * vn) + c.surfaceVel;
    e.onHit(*ball_, *this, c);
}

void SilverballGame::simulate(double dt) {
    if (state_ == State::Over || state_ == State::Bonus) return;
    elapsed_ += dt;
    for (auto& e : table_.elements) e->update(dt);
    float power = 0;
    if (table_.plunger->released(power) && state_ == State::Launch) {
        ball_->vel = {0, -(220.0f + power * 280.0f)};
        state_ = State::Play;
        ballSaveUntil_ = elapsed_ + kBallSave;
        timers_.push({ballSaveUntil_, Timer::BallSaveEnd});
        sfx("launch");
    }
    ball_->update(dt);
    for (auto& e : table_.elements) {
        if (const Collider* c = e->collider())
            if (const auto contact = c->test(*ball_)) resolve(*e, *contact);
        e->sense(*ball_, *this);
    }
    if (ball_->pos.y > table_.drainY) drained();
}

void SilverballGame::drained() {
    if (elapsed_ < ballSaveUntil_ && !tilted_ && state_ == State::Play) {
        ctx_.events.toast.emit("BALL SAVED");
        sfx("select");
        newBall();
        ballSaveLit_ = false;
        return;
    }
    sfx("drain");
    buzz(40);
    const std::array<std::uint32_t, 2> items{static_cast<std::uint32_t>(targetsHit_) * 50u,
                                             static_cast<std::uint32_t>(rolloversHit_) * 100u};
    bonus_ = tilted_ ? 0 : std::accumulate(items.begin(), items.end(), 0u) * static_cast<std::uint32_t>(multiplier_);
    score_ += bonus_;
    state_ = State::Bonus;
    bonusFrames_ = 0;
}

void SilverballGame::afterSubsteps(double) {
    if (state_ == State::Bonus && ++bonusFrames_ > 90) {
        if (--balls_ <= 0) {
            state_ = State::Over;
            return;
        }
        multiplier_ = 1;
        newBall();
    }
    while (!timers_.empty() && timers_.top().at <= elapsed_) {
        const Timed t = timers_.top();
        timers_.pop();
        if (t.what == Timer::BallSaveEnd) ballSaveLit_ = false;
        else if (t.what == Timer::LightsOut && !std::all_of(table_.rollovers.begin(), table_.rollovers.end(), [](auto* r) { return r->lit(); }))
            for (auto* r : table_.rollovers) r->unlight();
    }
    for (auto& p : sparks_) {
        p.pos += p.vel * (1.0f / 60.0f);
        p.vel.y += 4.0f;
        --p.life;
    }
    sparks_.remove_if([](const Particle& p) { return p.life <= 0; });
    const float target = std::clamp(ball_->pos.y - 80.0f, 0.0f, static_cast<float>(table_.size.y - 144));
    camera_.follow({0, target}, 0.18f);
}

void SilverballGame::drawWorld(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    r.setCamera(camera_.offset() - Vec2i{0, 8});
    r.setClip(Rect(0, 8, 160, 136));
    for (const auto& e : table_.elements) e->draw(r);
    ball_->draw(r);
    for (const auto& p : sparks_) r.pixel({static_cast<int>(p.pos.x), static_cast<int>(p.pos.y)}, Shade::Ink);
    if (state_ == State::Launch) {
        const Vec2f at = table_.plunger->restPosition();
        r.text("^", {static_cast<int>(at.x) - 2, static_cast<int>(at.y) - 40}, TextStyle{}.font(gfx::Font::Mini));
    }
}

void SilverballGame::drawHud(gfx::Renderer& r) const {
    r.fill(Rect(0, 0, 160, 8), Shade::Ink);
    char buf[24];
    std::snprintf(buf, sizeof buf, "%07u", static_cast<unsigned>(score_));
    const auto mini = TextStyle{}.font(gfx::Font::Mini).shade(Shade::Paper);
    r.text(buf, {2, 2}, mini);
    std::snprintf(buf, sizeof buf, "BALL %d", kBalls - balls_ + 1);
    r.text(buf, {44, 2}, mini);
    std::snprintf(buf, sizeof buf, "X%d", multiplier_);
    r.text(buf, {84, 2}, mini);
    if (tilted_) r.text("TILT", {104, 2}, mini);
    else if (ballSaveLit_ && state_ == State::Play) r.text("SAVE", {104, 2}, mini);
    if (state_ == State::Launch) r.text("HOLD DOWN", {158, 2}, TextStyle(mini).align(Align::Right));
    if (state_ == State::Bonus) {
        r.fill(Rect(30, 56, 100, 28), Shade::Paper);
        r.rect(Rect(30, 56, 100, 28), Shade::Ink);
        r.text("BONUS", {80, 60}, TextStyle{}.align(Align::Center));
        std::snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(bonus_));
        r.text(buf, {80, 72}, TextStyle{}.align(Align::Center));
    }
}

void SilverballGame::save(io::BinaryWriter& w) const {
    if (state_ == State::Bonus || state_ == State::Over) throw ContractViolation("save only during play");
    w << state_ << score_ << balls_ << multiplier_ << elapsed_ << ballSaveUntil_ << tilted_ << ballSaveLit_;
    w << targetsHit_ << rolloversHit_ << ball_->pos.x << ball_->pos.y << ball_->vel.x << ball_->vel.y << camera_.pos.y;
    for (const auto& e : table_.elements) e->saveState(w);
    auto timers = timers_;
    w << static_cast<std::uint8_t>(std::min<std::size_t>(timers.size(), 255));
    for (int i = 0; i < 255 && !timers.empty(); ++i, timers.pop()) w << timers.top().at << timers.top().what;
    w << std::vector<double>(nudges_.begin(), nudges_.end()) << ctx_.rng.state();
}

void SilverballGame::load(io::BinaryReader& r) {
    reset();
    r >> state_ >> score_ >> balls_ >> multiplier_ >> elapsed_ >> ballSaveUntil_ >> tilted_ >> ballSaveLit_;
    r >> targetsHit_ >> rolloversHit_ >> ball_->pos.x >> ball_->pos.y >> ball_->vel.x >> ball_->vel.y >> camera_.pos.y;
    if (state_ != State::Launch && state_ != State::Play) throw CorruptSaveError("SAVE CORRUPT", "state");
    for (auto& e : table_.elements) e->loadState(r);
    for (auto n = r.get<std::uint8_t>(); n > 0; --n) {
        Timed t{};
        r >> t.at >> t.what;
        timers_.push(t);
    }
    std::vector<double> nudges;
    r >> nudges;
    nudges_.assign(nudges.begin(), nudges.end());
    ctx_.rng.setState(r.get<std::string>());
    balls_ = std::clamp(balls_, 1, kBalls);
    multiplier_ = std::clamp(multiplier_, 1, 5);
}

}
