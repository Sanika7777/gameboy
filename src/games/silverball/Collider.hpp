#pragma once

#include "core/Vec2.hpp"
#include "games/GameObject.hpp"

#include <cmath>
#include <optional>

namespace nb::games::silverball {
[[nodiscard]] inline float dot(Vec2f a, Vec2f b) noexcept { return a.x * b.x + a.y * b.y; }
[[nodiscard]] inline float length(Vec2f v) noexcept { return std::sqrt(dot(v, v)); }

class Ball final : public GameObject {
public:
    static constexpr float kRadius = 3.0f;
    static constexpr float kMaxSpeed = 500.0f;

    Ball() : Identifiable("ball") {}
    void update(double dt) override;
    void draw(gfx::Renderer& r) const override;

    Vec2f pos, vel;
    float gravity = 300.0f;
};

struct Contact {
    Vec2f normal;
    float depth = 0;
    Vec2f point;
    Vec2f surfaceVel;
};

class Collider {
public:
    virtual ~Collider() = default;
    [[nodiscard]] virtual std::optional<Contact> test(const Ball& b) const = 0;
};

class CircleCollider final : public Collider {
public:
    CircleCollider(Vec2f centre, float radius) : c_(centre), r_(radius) {}
    [[nodiscard]] std::optional<Contact> test(const Ball& b) const override;

private:
    Vec2f c_;
    float r_;
};

class SegmentCollider : public Collider {
public:
    SegmentCollider(Vec2f a, Vec2f b, float thickness) : a_(a), b_(b), thick_(thickness) {}
    [[nodiscard]] std::optional<Contact> test(const Ball& b) const override;
    [[nodiscard]] Vec2f a() const noexcept { return a_; }
    [[nodiscard]] Vec2f b() const noexcept { return b_; }

protected:
    void setEnds(Vec2f a, Vec2f b) noexcept { a_ = a; b_ = b; }

private:
    Vec2f a_, b_;
    float thick_;
};

class FlipperCollider final : public SegmentCollider {
public:
    FlipperCollider(Vec2f pivot, float length, float side, float thickness);
    void setAngle(float angle, float angularVel) noexcept;
    [[nodiscard]] std::optional<Contact> test(const Ball& b) const override;
    [[nodiscard]] float angle() const noexcept { return angle_; }

private:
    Vec2f pivot_;
    float length_, side_;
    float angle_ = 0, angularVel_ = 0;
};

}
