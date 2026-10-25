#pragma once

#include "games/Game.hpp"
#include "io/IPersistence.hpp"

#include <string_view>

namespace nb::scenes {
[[nodiscard]] bool hasSave(std::string_view gameId);
void saveGame(const games::Game& g, io::IPersistence& p);
void loadGame(games::Game& g);
void deleteSave(std::string_view gameId, io::IPersistence& p) noexcept;

}
