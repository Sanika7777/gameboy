#pragma once

// Role: the base every cartridge implements; fixes the per-step algorithm.
// Demonstrates: abstract class, pure virtuals; Template Method (step/render); = delete;
//               static data members; friend class (GameTestAccess); virtual destructor

#include "core/Random.hpp"
#include "core/Seconds.hpp"
#include "core/Signal.hpp"
#include "gfx/AssetCache.hpp"
#include "gfx/Renderer.hpp"
#include "input/InputState.hpp"
#include "io/Binary.hpp"
#include "io/HighScoreTable.hpp"
#include "io/Settings.hpp"

#include <cstdint>
#include <optional>
#include <string>

namespace nb::games {

class GameTestAccess;   // defined by the tests; befriended by Game and each concrete game

struct GameInfo {
    std::string id;      // "serpent"
    std::string title;   // "SERPENT"
    std::string blurb;   // one line for the menu
    io::ScoreOrder order = io::ScoreOrder::HigherIsBetter;
};

// Everything a game may use from the outside world, injected so tests can supply their own.
struct GameContext {
    Random& rng;
    EventBus& events;
    gfx::AssetCache& assets;
    const io::SettingsValues& settings;
};

class Game {
public:
    explicit Game(GameContext& ctx) : ctx_(ctx) { ++liveInstances_; }
    virtual ~Game() { --liveInstances_; }
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Template Method: the order (input → world) is fixed here; subclasses fill in the steps.
    void step(const input::InputState& in, Seconds dt) {
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
    // The value for the high-score table, or nullopt if this run doesn't count (e.g. a lost MINEFIELD).
    [[nodiscard]] virtual std::optional<std::uint32_t> result() const { return score(); }
    [[nodiscard]] virtual std::string scoreTableKey() const { return info().id; }
    // False while a game shows its own setup screen (mode/level select): START isn't pause yet.
    [[nodiscard]] virtual bool isPlaying() const { return true; }

    [[nodiscard]] virtual bool supportsSaveState() const { return false; }
    virtual void save(io::BinaryWriter& w) const;
    virtual void load(io::BinaryReader& r);
    // Bumped by a game whenever its save payload layout changes.
    [[nodiscard]] virtual std::uint16_t saveVersion() const { return 1; }

    [[nodiscard]] static int liveInstances() noexcept { return liveInstances_; }

protected:
    virtual void handleInput(const input::InputState& in) = 0;
    virtual void updateWorld(Seconds dt) = 0;
    virtual void drawWorld(gfx::Renderer& r) const = 0;
    virtual void drawHud(gfx::Renderer& r) const;

    void sfx(std::string_view name) const { ctx_.events.playSfx.emit(name); }
    void buzz(int ms) const { ctx_.events.vibrate.emit(ms); }

    GameContext& ctx_;

private:
    static inline int liveInstances_ = 0;
    friend class GameTestAccess;
};

} // namespace nb::games
