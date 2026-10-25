#pragma once

// Role: power-on screen: the wordmark slides down, chimes, and waits for START.

#include "scenes/Scene.hpp"
#include "scenes/Services.hpp"

namespace nb::scenes {

class BootScene final : public Scene {
public:
    explicit BootScene(Services& s) : s_(s) {}
    void update(const input::InputState& in, Seconds dt) override;
    void draw(gfx::Renderer& r) const override;

private:
    Services& s_;
    int y_ = -12;
    int frames_ = 0, landedAt_ = -1;
};

} // namespace nb::scenes
