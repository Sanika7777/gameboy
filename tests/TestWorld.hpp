#pragma once

// Role: a headless GameContext for game tests: seeded Random, real assets, default settings.

#include "games/Game.hpp"
#include "io/Paths.hpp"

namespace nbtest {

struct TestWorld {
    explicit TestWorld(std::uint32_t seed = 1) : rng(seed) {
        nb::io::Paths::configure(NB_ASSET_DIR, std::filesystem::temp_directory_path());
    }
    nb::Random rng;
    nb::EventBus events;
    nb::gfx::AssetCache assets{NB_ASSET_DIR, {}};
    nb::io::SettingsValues settings;
    nb::games::GameContext ctx{rng, events, assets, settings};
};

// Steps a game N frames with the given input, clearing edges after each step like App does.
inline void run(nb::games::Game& g, nb::input::InputState& in, int frames) {
    for (int i = 0; i < frames; ++i) {
        g.step(in, nb::kStep);
        in.endStep();
    }
}

} // namespace nbtest
