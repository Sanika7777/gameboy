#pragma once

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
    void update(const input::InputState& in, double dt) override;
    void draw(gfx::Renderer& r) const override;

private:
    void refresh();
    void start(bool resume);
    [[nodiscard]] const std::string& currentId() const;

    Services& s_;
    std::vector<int> order_;
    gfx::SpriteSheet icons_;
    std::vector<std::optional<std::uint32_t>> best_;
    int frames_ = 0;
    int insertFrames_ = -1;
    bool resume_ = false;
    std::optional<ui::ListView> prompt_;
};

}
