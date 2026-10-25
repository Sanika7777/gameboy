#pragma once

// Role: builds a pinball table from a .tbl text file.
// Demonstrates: Factory (keyword → factory lambda in a std::map); ParseError with line numbers

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
    Flipper* leftFlipper = nullptr;    // non-owning views into elements
    Flipper* rightFlipper = nullptr;
    Plunger* plunger = nullptr;
    std::vector<DropTarget*> targets;
    std::vector<Rollover*> rollovers;
};

[[nodiscard]] Table loadTable(const std::filesystem::path& file);   // throws ParseError / FileNotFoundError

} // namespace nb::games::silverball
