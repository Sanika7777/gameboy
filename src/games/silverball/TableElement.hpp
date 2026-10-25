#pragma once

#include "games/silverball/Collider.hpp"
#include "io/Binary.hpp"

#include <memory>

namespace nb::games::silverball {
class SilverballGame;

class TableElement : public GameObject {
public:
    TableElement(std::unique_ptr<Collider> c, float restitution)
        : Identifiable("element"), collider_(std::move(c)), restitution_(restitution) {}

    [[nodiscard]] const Collider* collider() const noexcept { return active() ? collider_.get() : nullptr; }
    [[nodiscard]] float restitution() const noexcept { return restitution_; }
    [[nodiscard]] virtual bool active() const noexcept { return true; }
    virtual void onHit(Ball&, SilverballGame&, const Contact&) {}
    virtual void sense(const Ball&, SilverballGame&) {}
    void update(double) override {}
    virtual void saveState(io::BinaryWriter&) const {}
    virtual void loadState(io::BinaryReader&) {}

protected:
    std::unique_ptr<Collider> collider_;
    float restitution_;
};

class Wall final : public TableElement {
public:
    Wall(Vec2f a, Vec2f b);
    void draw(gfx::Renderer& r) const override;

private:
    Vec2f a_, b_;
};

class Bumper final : public TableElement {
public:
    Bumper(Vec2f c, float r, int score);
    void onHit(Ball& b, SilverballGame& rules, const Contact& c) override;
    void update(double) override { if (flash_ > 0) --flash_; }
    void draw(gfx::Renderer& r) const override;

private:
    Vec2f c_;
    float r_;
    int score_, flash_ = 0;
};

class Slingshot final : public TableElement {
public:
    Slingshot(Vec2f a, Vec2f b, int score);
    void onHit(Ball& b, SilverballGame& rules, const Contact& c) override;
    void update(double) override { if (flash_ > 0) --flash_; }
    void draw(gfx::Renderer& r) const override;

private:
    Vec2f a_, b_;
    int score_, flash_ = 0;
};

class DropTarget final : public TableElement {
public:
    DropTarget(Vec2f at, Vec2f size, int score, char group);
    [[nodiscard]] bool active() const noexcept override { return !down_; }
    void onHit(Ball& b, SilverballGame& rules, const Contact& c) override;
    void draw(gfx::Renderer& r) const override;
    void raise() noexcept { down_ = false; }
    [[nodiscard]] bool isDown() const noexcept { return down_; }
    [[nodiscard]] char group() const noexcept { return group_; }
    void saveState(io::BinaryWriter& w) const override { w << down_; }
    void loadState(io::BinaryReader& r) override { r >> down_; }

private:
    Vec2f at_, size_;
    int score_;
    char group_;
    bool down_ = false;
};

class Rollover final : public TableElement {
public:
    Rollover(Vec2f at, int lane);
    void sense(const Ball& b, SilverballGame& rules) override;
    void draw(gfx::Renderer& r) const override;
    [[nodiscard]] bool lit() const noexcept { return lit_; }
    void unlight() noexcept { lit_ = false; }
    void saveState(io::BinaryWriter& w) const override { w << lit_ << inside_; }
    void loadState(io::BinaryReader& r) override { r >> lit_ >> inside_; }

private:
    Vec2f at_;
    int lane_;
    bool lit_ = false, inside_ = false;
};

class Flipper final : public TableElement {
public:
    static constexpr float kRest = 0.52f, kUp = -0.52f, kSpeed = 25.0f;
    Flipper(bool left, Vec2f pivot, float length);
    void setPressed(bool on) noexcept { pressed_ = on; }
    void update(double dt) override;
    void draw(gfx::Renderer& r) const override;
    void saveState(io::BinaryWriter& w) const override { w << angle_; }
    void loadState(io::BinaryReader& r) override;

private:
    FlipperCollider* shape_;
    Vec2f pivot_;
    float length_, side_, angle_ = kRest;
    bool pressed_ = false;
};

class Plunger final : public TableElement {
public:
    explicit Plunger(Vec2f at);
    void setPulling(bool on) noexcept { pulling_ = on; }
    void update(double dt) override;
    void draw(gfx::Renderer& r) const override;
    [[nodiscard]] bool released(float& power) noexcept;
    [[nodiscard]] Vec2f restPosition() const noexcept { return {at_.x, at_.y - Ball::kRadius - 1}; }
    void saveState(io::BinaryWriter& w) const override { w << pull_; }
    void loadState(io::BinaryReader& r) override { r >> pull_; }

private:
    Vec2f at_;
    float pull_ = 0;
    bool pulling_ = false, wasPulling_ = false;
};

}
