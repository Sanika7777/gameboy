#pragma once

#include "io/HighScoreTable.hpp"
#include "scenes/Scene.hpp"
#include "scenes/Services.hpp"

#include <array>
#include <optional>
#include <string>

namespace nb::scenes {
class ScoreEntryScene final : public Scene {
public:
    ScoreEntryScene(Services& s, std::string key, io::ScoreOrder order, std::uint32_t value, std::string title);
    void update(const input::InputState& in, double dt) override;
    void draw(gfx::Renderer& r) const override;

private:
    void submit();

    Services& s_;
    std::string key_, title_;
    io::ScoreOrder order_;
    std::uint32_t value_;
    std::array<char, 3> initials_{'A', 'A', 'A'};
    int pos_ = 0, frames_ = 0;
    std::optional<std::size_t> rank_;
    bool done_ = false;
};

}
