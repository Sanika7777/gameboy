#include "games/flapwing/FlapwingGame.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace nb::games::flapwing {
using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

void Bird::update(double dt) {
    const float t = dt;
    vel.y = std::min(vel.y + Physics::kGravity * t, Physics::kTerminal);
    pos.y += vel.y * t;
    if (pos.y < -8) {
        pos.y = -8;
        vel.y = 0;
    }
}

void Bird::draw(gfx::Renderer& r) const {
    r.draw(sprites_.get(vel.y < 0 ? "bird_down" : "bird_up"), {static_cast<int>(pos.x), static_cast<int>(pos.y)});
}

void ParallaxLayer::draw(gfx::Renderer& r) const {
    const int o = static_cast<int>(offset);
    switch (look_) {
    case Look::Clouds:
        for (int i = 0; i < 4; ++i) {
            const int x = ((i * 53 - o) % 212 + 212) % 212 - 26;
            const int y = 14 + (i * 17) % 30;
            r.fillCircle({x, y}, 5, Shade::Light);
            r.fillCircle({x + 7, y + 1}, 6, Shade::Light);
            r.fillCircle({x + 14, y}, 4, Shade::Light);
        }
        break;
    case Look::Hills:
        for (int x = 0; x < 160; ++x) {
            const float wx = static_cast<float>(x + o);
            const int h = 14 + static_cast<int>(8 * std::sin(wx * 0.045f) + 5 * std::sin(wx * 0.11f));
            r.line({x, Physics::kGround - h}, {x, Physics::kGround - 1}, Shade::Light);
        }
        break;
    case Look::Ground:
        r.fill(Rect(0, Physics::kGround, 160, 16), Shade::Dark);
        r.line({0, Physics::kGround}, {159, Physics::kGround}, Shade::Ink);
        for (int x = -(o % 8); x < 160; x += 8) r.line({x, Physics::kGround + 3}, {x + 3, Physics::kGround + 6}, Shade::Ink);
        break;
    }
}

const GameInfo& FlapwingGame::staticInfo() {
    static const GameInfo info{"flapwing", "FLAPWING", "TAP TO FLY, MIND THE PIPES", io::ScoreOrder::HigherIsBetter};
    return info;
}

FlapwingGame::FlapwingGame(GameContext& ctx) : PhysicsGame(ctx, 2), sprites_(gfx::SpriteSheet::load(io::Paths::asset("sprites/flapwing.spr"))) {
    auto clouds = std::make_unique<ParallaxLayer>(ParallaxLayer::Look::Clouds, 0.2f, speed_);
    auto hills = std::make_unique<ParallaxLayer>(ParallaxLayer::Look::Hills, 0.5f, speed_);
    auto bird = std::make_unique<Bird>(sprites_);
    auto ground = std::make_unique<ParallaxLayer>(ParallaxLayer::Look::Ground, 1.0f, speed_);
    clouds_ = clouds.get();
    hills_ = hills.get();
    bird_ = bird.get();
    ground_ = ground.get();
    objects_.push_back(std::move(clouds));
    objects_.push_back(std::move(hills));
    objects_.push_back(std::move(bird));
    objects_.push_back(std::move(ground));
    pipes_.resize(kPipes);
    reset();
}

void FlapwingGame::resetPipe(PipePair& p, float x) {
    p.x = x;
    p.gap = 44 - std::min(10, static_cast<int>(score_) / 4);
    p.gapY = ctx_.rng.range(16 + p.gap / 2, Physics::kGround - 12 - p.gap / 2);
    p.scored = false;
}

void FlapwingGame::reset() {
    score_ = 0;
    speed_ = 50.0f;
    bird_->pos = {40, 60};
    bird_->vel = {};
    for (int i = 0; i < kPipes; ++i) resetPipe(pipes_[static_cast<std::size_t>(i)], 200.0f + i * kSpacing);
    state_ = State::Ready;
    overFrames_ = 0;
}

void FlapwingGame::handleInput(const input::InputState& in) {
    ++frames_;
    const bool flap = in.pressed(Button::A) || in.pressed(Button::Up) || in.lcdTap().has_value();
    if (!flap) return;
    if (state_ == State::Ready) state_ = State::Flying;
    if (state_ != State::Flying) return;
    bird_->flap();
    sfx("flap");
}

bool FlapwingGame::collides() const {
    const Rect b = bird_->hitbox();
    if (b.bottom() >= Physics::kGround) return true;
    for (const auto& p : pipes_) {
        const int x = static_cast<int>(p.x);
        const Rect top(x, -100, PipePair::kWidth, 100 + p.gapY - p.gap / 2);
        const Rect bottom(x, p.gapY + p.gap / 2, PipePair::kWidth, Physics::kGround);
        if (!(b & top).empty() || !(b & bottom).empty()) return true;
    }
    return false;
}

void FlapwingGame::simulate(double dt) {
    switch (state_) {
    case State::Ready:
        bird_->pos.y = 60 + 3 * std::sin(frames_ * 0.1f);
        clouds_->update(dt);
        hills_->update(dt);
        ground_->update(dt);
        return;
    case State::Over: return;
    case State::Falling:
        bird_->update(dt);
        if (bird_->pos.y >= Physics::kGround - 8) {
            bird_->pos.y = Physics::kGround - 8;
            state_ = State::Over;
        }
        return;
    case State::Flying: break;
    }
    for (auto& o : objects_) o->update(dt);
    for (auto& p : pipes_) {
        p.x -= speed_ * dt;
        if (!p.scored && p.x + PipePair::kWidth < bird_->pos.x) {
            p.scored = true;
            ++score_;
            sfx("point");
            if (score_ == 10 || score_ == 20 || score_ == 40) sfx("medal");
        }
    }
    if (pipes_.front().x < -PipePair::kWidth) {
        const float x = pipes_.back().x + kSpacing;
        std::rotate(pipes_.begin(), pipes_.begin() + 1, pipes_.end());
        resetPipe(pipes_.back(), x);
    }
    if (collides()) {
        state_ = State::Falling;
        bird_->vel.y = std::max(bird_->vel.y, 0.0f);
        sfx("hit");
        buzz(40);
    }
}

void FlapwingGame::afterSubsteps(double) {
    speed_ = 50.0f + std::min(25.0f, static_cast<float>(score_));
}

void FlapwingGame::drawWorld(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    clouds_->draw(r);
    hills_->draw(r);
    for (const auto& p : pipes_) {
        const int x = static_cast<int>(p.x);
        const int top = p.gapY - p.gap / 2, bottom = p.gapY + p.gap / 2;
        r.fill(Rect(x, 0, PipePair::kWidth, top), Shade::Dark);
        r.fill(Rect(x - 2, top - 6, PipePair::kWidth + 4, 6), Shade::Dark);
        r.rect(Rect(x - 2, top - 6, PipePair::kWidth + 4, 6), Shade::Ink);
        r.fill(Rect(x, bottom, PipePair::kWidth, Physics::kGround - bottom), Shade::Dark);
        r.fill(Rect(x - 2, bottom, PipePair::kWidth + 4, 6), Shade::Dark);
        r.rect(Rect(x - 2, bottom, PipePair::kWidth + 4, 6), Shade::Ink);
        r.line({x + 3, 0}, {x + 3, top - 7}, Shade::Light);
        r.line({x + 3, bottom + 6}, {x + 3, Physics::kGround - 1}, Shade::Light);
    }
    bird_->draw(r);
    ground_->draw(r);

    if ((score_ / 25) % 2 == 1) {
        auto& px = r.target();
        for (int y = 0; y < Physics::kGround; ++y)
            for (auto& s : px.row(y)) s = gfx::invert(s);
    }
}

void FlapwingGame::drawHud(gfx::Renderer& r) const {
    char buf[8];
    std::snprintf(buf, sizeof buf, "%u", static_cast<unsigned>(score_));
    const bool night = (score_ / 25) % 2 == 1;
    r.text(buf, {80, 6}, TextStyle{}.align(Align::Center).inverted().shade(night ? Shade::Paper : Shade::Ink));
    if (state_ == State::Ready) {
        r.text("GET READY", {80, 36}, TextStyle{}.align(Align::Center));
        r.text("TAP, A OR UP TO FLAP", {80, 100}, TextStyle{}.font(gfx::Font::Mini).align(Align::Center));
    }
    if (state_ == State::Over && score_ >= 10) {
        const char* medal = score_ >= 40 ? "medal_gold" : score_ >= 20 ? "medal_silver" : "medal_bronze";
        r.draw(sprites_.get(medal), {74, 24});
    }
}

void FlapwingGame::save(io::BinaryWriter& w) const {
    w << state_ << score_ << speed_ << bird_->pos.x << bird_->pos.y << bird_->vel.y;
    w << clouds_->offset << hills_->offset << ground_->offset;
    for (const auto& p : pipes_) w << p.x << p.gapY << p.gap << p.scored;
    w << ctx_.rng.state();
}

void FlapwingGame::load(io::BinaryReader& r) {
    r >> state_ >> score_ >> speed_ >> bird_->pos.x >> bird_->pos.y >> bird_->vel.y;
    r >> clouds_->offset >> hills_->offset >> ground_->offset;
    for (auto& p : pipes_) r >> p.x >> p.gapY >> p.gap >> p.scored;
    ctx_.rng.setState(r.get<std::string>());
    if (static_cast<int>(state_) > 3) throw CorruptSaveError("SAVE CORRUPT", "state");
}

}
