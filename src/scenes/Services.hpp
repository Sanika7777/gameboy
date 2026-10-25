#pragma once

// Role: the app-wide objects scenes need, bundled so constructors stay short.

#include "core/Random.hpp"
#include "core/Signal.hpp"
#include "gfx/AssetCache.hpp"
#include "io/HighScoreTable.hpp"
#include "io/IPersistence.hpp"
#include "io/Settings.hpp"

namespace nb::games { class GameRegistry; }

namespace nb::scenes {

class SceneStack;

struct Services {
    SceneStack& scenes;
    EventBus& events;
    Random& rng;
    gfx::AssetCache& assets;
    io::Settings& settings;
    io::ScoreBook& scores;
    const games::GameRegistry& registry;
    io::IPersistence& persistence;

    void sfx(std::string_view name) const { events.playSfx.emit(name); }
    void toast(std::string_view msg) const { events.toast.emit(msg); }
    // Saves scores; failures become a toast, never a crash.
    void saveScores() const;
};

} // namespace nb::scenes
