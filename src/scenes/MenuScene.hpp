#pragma once

// Role: the cartridge carousel.

#include "scenes/Scene.hpp"
#include "scenes/Services.hpp"
#include "ui/Widget.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace nb::scenes {

class MenuScene final : public Scene {
public:
    explicit MenuScene(Services& s);
    void onEnter() override { refresh(); }
    void onResume() override { refresh(); }
    void update(const input::InputState& in, Seconds dt) override;
    void draw(gfx::Renderer& r) const override;

private:
    void refresh();
    void start(bool resume);
    [[nodiscard]] const std::string& currentId() const;

    Services& s_;
    std::vector<int> order_;     // catalog indices; front() is the cartridge on show
    std::shared_ptr<const gfx::SpriteSheet> icons_;
    std::vector<std::optional<std::uint32_t>> best_;
    int frames_ = 0;
    int insertFrames_ = -1;      // ≥0 while the cartridge slides into the slot
    bool resume_ = false;
    std::optional<ui::ListView> prompt_;   // CONTINUE? when a save state exists
};

} // namespace nb::scenes
