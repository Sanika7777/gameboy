#include "games/GameRegistry.hpp"

#include "games/flapwing/FlapwingGame.hpp"
#include "games/minefield/MinefieldGame.hpp"
#include "games/serpent/SerpentGame.hpp"
#include "games/silverball/SilverballGame.hpp"
#include "games/stackfall/StackfallGame.hpp"

namespace nb::games {
namespace {
template<class G>
void add(GameRegistry& reg) {
    reg.add(G::staticInfo(), [](GameContext& ctx) -> std::unique_ptr<Game> { return std::make_unique<G>(ctx); });
}

}

void registerAll(GameRegistry& registry) {
    add<serpent::SerpentGame>(registry);
    add<minefield::MinefieldGame>(registry);
    add<stackfall::StackfallGame>(registry);
    add<flapwing::FlapwingGame>(registry);
    add<silverball::SilverballGame>(registry);
}

}
