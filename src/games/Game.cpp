#include "games/Game.hpp"

#include "scenes/Common.hpp"

namespace nb::games {
void Game::save(io::BinaryWriter&) const { throw ContractViolation(info().id + " has no save state"); }
void Game::load(io::BinaryReader&) { throw ContractViolation(info().id + " has no save state"); }

void Game::drawHud(gfx::Renderer& r) const {
    r.text(scenes::fmt("%u", static_cast<unsigned>(score())).data(), {2, 2}, gfx::TextStyle{}.font(gfx::Font::Mini));
}

}
