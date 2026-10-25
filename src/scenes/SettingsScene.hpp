#pragma once

// Role: settings menu with sub-pages; every change applies (and saves) immediately.
// Demonstrates: std::stack (sub-menu breadcrumb)

#include "scenes/Scene.hpp"
#include "scenes/Services.hpp"
#include "ui/Widget.hpp"

#include <stack>
#include <string>
#include <vector>

namespace nb::scenes {

class SettingsScene final : public Scene {
public:
    explicit SettingsScene(Services& s);
    void update(const input::InputState& in, Seconds dt) override;
    void draw(gfx::Renderer& r) const override;

    enum class Page { Main, Display, Sound, Controls, Reset, About };

private:
    void open(Page p);
    void rebuild();
    void change(int item, int dir);

    Services& s_;
    // std::stack: only the current page and "back" matter, never random access.
    std::stack<Page> pages_;
    ui::ListView list_;
};

} // namespace nb::scenes
