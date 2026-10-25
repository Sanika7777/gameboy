#pragma once

#include "core/Vec2.hpp"
#include "input/Button.hpp"

#include <optional>

namespace nb::input {
struct Hit {
    std::optional<Button> button;
    bool dpadDeadZone = false;
    std::optional<Vec2i> lcd;
};

class IHitTest {
public:
    virtual ~IHitTest() = default;
    [[nodiscard]] virtual Hit hit(Vec2i phys) const = 0;
};

}
