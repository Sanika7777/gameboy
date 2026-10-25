#pragma once

#include "core/Random.hpp"
#include "core/Signal.hpp"
#include "gfx/Sprite.hpp"
#include "io/Paths.hpp"
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
    io::Settings& settings;
    io::ScoreBook& scores;
    const games::GameRegistry& registry;
    io::IPersistence& persistence;

    void sfx(std::string_view name) const { events.playSfx.emit(name); }
    void toast(std::string_view msg) const { events.toast.emit(msg); }
    void saveScores() const;
};

}
