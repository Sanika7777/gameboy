#pragma once

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
    void update(const input::InputState& in, double dt) override;
    void draw(gfx::Renderer& r) const override;

    [[nodiscard]] const std::vector<std::string>& lines() const noexcept { return lines_; }

private:
    void describe(const std::exception& e);

    Services& s_;
    std::vector<std::string> lines_;
};

}
