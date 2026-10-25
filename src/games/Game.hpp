#pragma once

#include "core/Random.hpp"
#include "core/Clock.hpp"
#include "core/Signal.hpp"
#include "gfx/Sprite.hpp"
#include "io/Paths.hpp"
#include "gfx/Renderer.hpp"
#include "input/InputState.hpp"
#include "io/Binary.hpp"
#include "io/HighScoreTable.hpp"
#include "io/Settings.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace nb::games {
class GameTestAccess;

struct GameInfo {
    std::string id;
    std::string title;
    std::string blurb;
    io::ScoreOrder order = io::ScoreOrder::HigherIsBetter;
};

struct GameContext {
    Random& rng;
    EventBus& events;
    const io::SettingsValues& settings;
};

class Game {
public:
    explicit Game(GameContext& ctx) : ctx_(ctx) { ++liveInstances_; }
    virtual ~Game() { --liveInstances_; }
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void step(const input::InputState& in, double dt) {
        handleInput(in);
        updateWorld(dt);
    }
    void render(gfx::Renderer& r) const {
        drawWorld(r);
        r.setCamera({0, 0});
        r.resetClip();
        drawHud(r);
    }

    [[nodiscard]] virtual const GameInfo& info() const = 0;
    virtual void reset() = 0;
    [[nodiscard]] virtual bool isOver() const = 0;
    [[nodiscard]] virtual std::uint32_t score() const = 0;
    [[nodiscard]] virtual std::optional<std::uint32_t> result() const { return score(); }
    [[nodiscard]] virtual std::string scoreTableKey() const { return info().id; }
    [[nodiscard]] virtual bool isPlaying() const { return true; }

    [[nodiscard]] virtual bool supportsSaveState() const { return false; }
    virtual void save(io::BinaryWriter& w) const;
    virtual void load(io::BinaryReader& r);
    [[nodiscard]] virtual std::uint16_t saveVersion() const { return 1; }

    [[nodiscard]] static int liveInstances() noexcept { return liveInstances_; }

protected:
    virtual void handleInput(const input::InputState& in) = 0;
    virtual void updateWorld(double dt) = 0;
    virtual void drawWorld(gfx::Renderer& r) const = 0;
    virtual void drawHud(gfx::Renderer& r) const;

    void sfx(std::string_view name) const { ctx_.events.playSfx.emit(name); }
    void buzz(int ms) const { ctx_.events.vibrate.emit(ms); }

    GameContext& ctx_;

private:
    static inline int liveInstances_ = 0;
    friend class GameTestAccess;
};

}
