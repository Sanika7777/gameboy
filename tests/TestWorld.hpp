#pragma once

#include "games/Game.hpp"
#include "io/Paths.hpp"

namespace nbtest {
struct TestWorld {
    explicit TestWorld(std::uint32_t seed = 1) : rng(seed) {
        nb::io::Paths::configure(NB_ASSET_DIR, std::filesystem::temp_directory_path());
    }
    nb::Random rng;
    nb::EventBus events;
    nb::io::SettingsValues settings;
    nb::games::GameContext ctx{rng, events, settings};
};

inline void run(nb::games::Game& g, nb::input::InputState& in, int frames) {
    for (int i = 0; i < frames; ++i) {
        g.step(in, nb::kStep);
        in.endStep();
    }
}

}
