#pragma once

// Role: lets touch/mouse sources ask "what is under this point?" without knowing the layout.

#include "core/Vec2.hpp"
#include "input/Button.hpp"

#include <optional>

namespace nb::input {

struct Hit {
    std::optional<Button> button;
    bool dpadDeadZone = false;        // on the D-pad centre: keep the previous direction
    std::optional<Vec2i> lcd;         // point on the LCD, in LCD pixels
};

class IHitTest {
public:
    virtual ~IHitTest() = default;
    [[nodiscard]] virtual Hit hit(Vec2i phys) const = 0;
};

} // namespace nb::input
