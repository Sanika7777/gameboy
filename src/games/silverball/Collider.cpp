#include "games/silverball/Collider.hpp"

#include <algorithm>

namespace nb::games::silverball {
void Ball::update(double dt) {
    const float t = dt;
    vel.y += gravity * t;
    const float speed = length(vel);
    if (speed > kMaxSpeed) vel = vel * (kMaxSpeed / speed);
    pos += vel * t;
}

void Ball::draw(gfx::Renderer& r) const {
    const Vec2i p{static_cast<int>(std::lround(pos.x)), static_cast<int>(std::lround(pos.y))};
    r.fillCircle(p, 3, gfx::Shade::Ink);
    r.pixel(p + Vec2i{-1, -1}, gfx::Shade::Light);
}

std::optional<Contact> CircleCollider::test(const Ball& b) const {
    const Vec2f d = b.pos - c_;
    const float dist = length(d);
    const float reach = r_ + Ball::kRadius;
    if (dist >= reach) return std::nullopt;
    const Vec2f n = dist > 1e-4f ? d * (1.0f / dist) : Vec2f{0, -1};
    return Contact{n, reach - dist, c_ + n * r_, {}};
}

std::optional<Contact> SegmentCollider::test(const Ball& ball) const {
    const Vec2f ab = b_ - a_;
    const float len2 = dot(ab, ab);
    const float t = len2 > 0 ? std::clamp(dot(ball.pos - a_, ab) / len2, 0.0f, 1.0f) : 0.0f;
    const Vec2f q = a_ + ab * t;
    const Vec2f d = ball.pos - q;
    const float dist = length(d);
    const float reach = thick_ + Ball::kRadius;
    if (dist >= reach) return std::nullopt;
    const Vec2f n = dist > 1e-4f ? d * (1.0f / dist) : Vec2f{-ab.y, ab.x} * (1.0f / std::sqrt(len2));
    return Contact{n, reach - dist, q, {}};
}

FlipperCollider::FlipperCollider(Vec2f pivot, float len, float side, float thickness)
    : SegmentCollider(pivot, pivot, thickness), pivot_(pivot), length_(len), side_(side) {
    setAngle(0, 0);
}

void FlipperCollider::setAngle(float angle, float angularVel) noexcept {
    angle_ = angle;
    angularVel_ = angularVel;
    setEnds(pivot_, pivot_ + Vec2f{side_ * std::cos(angle), std::sin(angle)} * length_);
}

std::optional<Contact> FlipperCollider::test(const Ball& b) const {
    auto c = SegmentCollider::test(b);
    if (!c) return c;
    const float r = length(c->point - pivot_);
    c->surfaceVel = Vec2f{-side_ * std::sin(angle_), std::cos(angle_)} * (r * angularVel_);
    return c;
}

}
