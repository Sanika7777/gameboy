#pragma once

// Role: the one place that knows every cartridge; App calls it once at start-up.

#include "games/GameRegistry.hpp"

namespace nb::games {

void registerAll(GameRegistry& registry);

} // namespace nb::games
