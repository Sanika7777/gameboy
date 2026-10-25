#pragma once

// Role: shows what went wrong (the whole nested chain) and returns to the menu.
// Demonstrates: RTTI and dynamic_cast (ParseError details); nested exceptions (rethrow_if_nested)

#include "scenes/Scene.hpp"
#include "scenes/Services.hpp"

#include <exception>
#include <string>
#include <vector>

namespace nb::scenes {

class ErrorScene final : public Scene {
public:
    ErrorScene(Services& s, std::exception_ptr error);
    void onEnter() override;
    void update(const input::InputState& in, Seconds dt) override;
    void draw(gfx::Renderer& r) const override;

    [[nodiscard]] const std::vector<std::string>& lines() const noexcept { return lines_; }

private:
    void describe(const std::exception& e);

    Services& s_;
    std::vector<std::string> lines_;   // already wrapped to 26 columns
};

} // namespace nb::scenes
