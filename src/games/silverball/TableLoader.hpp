#pragma once

#include "games/silverball/TableElement.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace nb::games::silverball {
struct Table {
    std::string name;
    Vec2i size{160, 288};
    float gravity = 300.0f;
    float drainY = 284.0f;
    std::vector<std::unique_ptr<TableElement>> elements;
    Flipper* leftFlipper = nullptr;
    Flipper* rightFlipper = nullptr;
    Plunger* plunger = nullptr;
    std::vector<DropTarget*> targets;
    std::vector<Rollover*> rollovers;
};

[[nodiscard]] Table loadTable(const std::filesystem::path& file);

}
