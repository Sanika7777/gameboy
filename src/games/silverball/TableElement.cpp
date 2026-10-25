#include "games/silverball/TableElement.hpp"

#include <algorithm>

namespace nb::games::silverball {

using gfx::Shade;

namespace {

Vec2i px(Vec2f v) { return {static_cast<int>(std::lround(v.x)), static_cast<int>(std::lround(v.y))}; }

// Kicks the ball away from a surface so it leaves at least `minSpeed` along the normal.
void kick(Ball& b, Vec2f n, float minSpeed) {
    const float along = dot(b.vel, n);
    if (along < minSpeed) b.vel += n * (minSpeed - along);
}

} // namespace

Wall::Wall(Vec2f a, Vec2f b) : Identifiable("wall"), TableElement(std::make_unique<SegmentCollider>(a, b, 1.0f), 0.5f), a_(a), b_(b) {}
void Wall::draw(gfx::Renderer& r) const { r.line(px(a_), px(b_), Shade::Ink); }

Bumper::Bumper(Vec2f c, float rad, int score)
    : Identifiable("bumper"), TableElement(std::make_unique<CircleCollider>(c, rad), 0.9f), c_(c), r_(rad), score_(score) {}

void Bumper::onHit(Ball& b, Rules& rules, const Contact& c) {
    kick(b, c.normal, 180.0f);
    flash_ = 8;
    rules.award(score_);
    rules.sound("bumper");
    rules.spark(c.point);
}

void Bumper::draw(gfx::Renderer& r) const {
    const int rad = static_cast<int>(r_);
    r.fillCircle(px(c_), rad, flash_ > 0 ? Shade::Ink : Shade::Light);
    r.circle(px(c_), rad, Shade::Ink);
    r.circle(px(c_), rad - 3, flash_ > 0 ? Shade::Paper : Shade::Dark);
}

Slingshot::Slingshot(Vec2f a, Vec2f b, int score)
    : Identifiable("sling"), TableElement(std::make_unique<SegmentCollider>(a, b, 1.5f), 0.6f), a_(a), b_(b), score_(score) {}

void Slingshot::onHit(Ball& b, Rules& rules, const Contact& c) {
    kick(b, c.normal, 150.0f);
    flash_ = 6;
    rules.award(score_);
    rules.sound("sling");
}

void Slingshot::draw(gfx::Renderer& r) const {
    const Shade s = flash_ > 0 ? Shade::Dark : Shade::Ink;
    r.line(px(a_), px(b_), s);
    r.line(px(a_) + Vec2i{1, 0}, px(b_) + Vec2i{1, 0}, s);
}

DropTarget::DropTarget(Vec2f at, Vec2f size, int score, char group)
    : Identifiable("target"),
      TableElement(std::make_unique<SegmentCollider>(at + Vec2f{0, size.y / 2}, at + Vec2f{size.x, size.y / 2}, size.y / 2), 0.4f),
      at_(at), size_(size), score_(score), group_(group) {}

void DropTarget::onHit(Ball&, Rules& rules, const Contact& c) {
    down_ = true;
    rules.award(score_);
    rules.sound("target");
    rules.spark(c.point);
    rules.targetDown(group_);
}

void DropTarget::draw(gfx::Renderer& r) const {
    const Rect b(px(at_).x, px(at_).y, static_cast<int>(size_.x), static_cast<int>(size_.y));
    if (down_) r.line({b.x, b.bottom() - 1}, {b.right() - 1, b.bottom() - 1}, Shade::Light);
    else {
        r.fill(b, Shade::Dark);
        r.rect(b, Shade::Ink);
    }
}

Rollover::Rollover(Vec2f at, int lane) : Identifiable("rollover"), TableElement(nullptr, 0), at_(at), lane_(lane) {}

void Rollover::sense(const Ball& b, Rules& rules) {
    const bool in = length(b.pos - at_) < 5.0f;
    if (in && !inside_ && !lit_) {
        lit_ = true;
        rules.award(100);
        rules.sound("rollover");
        rules.rolloverLit();
    }
    inside_ = in;
}

void Rollover::draw(gfx::Renderer& r) const {
    if (lit_) r.fillCircle(px(at_), 2, Shade::Ink);
    else r.circle(px(at_), 2, Shade::Dark);
    (void)lane_;
}

Flipper::Flipper(bool left, Vec2f pivot, float len)
    : Identifiable(left ? "flipper.left" : "flipper.right"),
      TableElement(std::make_unique<FlipperCollider>(pivot, len, left ? 1.0f : -1.0f, 2.0f), 0.3f),
      pivot_(pivot), length_(len), side_(left ? 1.0f : -1.0f) {
    shape_ = static_cast<FlipperCollider*>(collider_.get());
    shape_->setAngle(angle_, 0);
}

void Flipper::update(Seconds dt) {
    const float target = pressed_ ? kUp : kRest;
    const float step = kSpeed * dt.asFloat();
    const float before = angle_;
    angle_ = angle_ < target ? std::min(angle_ + step, target) : std::max(angle_ - step, target);
    shape_->setAngle(angle_, (angle_ - before) / dt.asFloat());
}

void Flipper::loadState(io::BinaryReader& r) {
    r >> angle_;
    angle_ = std::clamp(angle_, kUp, kRest);
    shape_->setAngle(angle_, 0);
}

void Flipper::draw(gfx::Renderer& r) const {
    const Vec2f tip = pivot_ + Vec2f{side_ * std::cos(angle_), std::sin(angle_)} * length_;
    for (int o = -1; o <= 1; ++o) r.line(px(pivot_) + Vec2i{0, o}, px(tip) + Vec2i{0, o}, o == 0 ? Shade::Dark : Shade::Ink);
    r.fillCircle(px(pivot_), 2, Shade::Ink);
}

Plunger::Plunger(Vec2f at)
    : Identifiable("plunger"), TableElement(std::make_unique<SegmentCollider>(at + Vec2f{-4, 0}, at + Vec2f{4, 0}, 1.0f), 0.1f), at_(at) {}

void Plunger::update(Seconds dt) {
    if (pulling_) pull_ = std::min(1.0f, pull_ + dt.asFloat() * 1.2f);
}

bool Plunger::released(float& power) noexcept {
    const bool fired = wasPulling_ && !pulling_ && pull_ > 0;
    wasPulling_ = pulling_;
    if (!fired) return false;
    power = pull_;
    pull_ = 0;
    return true;
}

void Plunger::draw(gfx::Renderer& r) const {
    const int y = static_cast<int>(at_.y + pull_ * 8);
    const int x = static_cast<int>(at_.x);
    r.fill(Rect(x - 4, y, 9, 2), Shade::Ink);
    for (int k = y + 3; k < static_cast<int>(at_.y) + 16; k += 2) r.line({x - 2, k}, {x + 2, k}, Shade::Dark);
}

} // namespace nb::games::silverball
