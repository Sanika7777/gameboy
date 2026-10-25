#pragma once

// Role: suspend/resume files (<game>.sav, design §10.5).

#include "games/Game.hpp"
#include "io/IPersistence.hpp"

#include <string_view>

namespace nb::scenes {

[[nodiscard]] bool hasSave(std::string_view gameId);
void saveGame(const games::Game& g, io::IPersistence& p);   // throws SaveError
void loadGame(games::Game& g);                              // throws SaveError subclasses
void deleteSave(std::string_view gameId, io::IPersistence& p) noexcept;

} // namespace nb::scenes
