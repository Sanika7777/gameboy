#include "games/Game.hpp"

#include <array>
#include <cstdio>

namespace nb::games {

void Game::save(io::BinaryWriter&) const { throw ContractViolation(info().id + " has no save state"); }
void Game::load(io::BinaryReader&) { throw ContractViolation(info().id + " has no save state"); }

void Game::drawHud(gfx::Renderer& r) const {
    std::array<char, 16> buf{};
    std::snprintf(buf.data(), buf.size(), "%u", static_cast<unsigned>(score()));
    r.text(buf.data(), {2, 2}, gfx::TextStyle{}.font(gfx::Font::Mini));
}

} // namespace nb::games
