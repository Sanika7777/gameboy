#include "scenes/GameScene.hpp"

#include "core/Log.hpp"
#include "games/GameRegistry.hpp"
#include "scenes/Common.hpp"
#include "scenes/PauseScene.hpp"
#include "scenes/SaveState.hpp"
#include "scenes/ScoreEntryScene.hpp"

#include <exception>

namespace nb::scenes {

using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

GameScene::GameScene(Services& s, const std::string& gameId, bool resume)
    : s_(s), ctx_{s.rng, s.events, s.assets, s.settings.get()} {
    const auto* info = s.registry.find(gameId);
    try {
        game_ = s.registry.create(gameId, ctx_);
    } catch (const AssetError&) {
        // Keep the asset detail (file:line) and add which cartridge failed on top.
        std::throw_with_nested(GameError("CAN'T START " + (info ? info->title : gameId)));
    }
    if (!resume) return;
    try {
        loadGame(*game_);
    } catch (const SaveError& e) {
        logWarn(gameId, ".sav: ", e.what(), " ", e.context());
        s_.toast("SAVE UNREADABLE");
        game_->reset();
    }
    deleteSave(gameId, s_.persistence);   // a save state is used once
}

GameScene::~GameScene() = default;

void GameScene::update(const input::InputState& in, Seconds dt) {
    if (overFrames_ >= 0) {
        ++overFrames_;
        if (overFrames_ > 150 || (overFrames_ > 40 && (in.pressed(Button::A) || in.pressed(Button::Start) || in.lcdTap()))) finish();
        return;
    }
    if (in.pressed(Button::Start) && game_->isPlaying()) {
        s_.sfx("pause");
        s_.scenes.push<PauseScene>(s_, *this);
        return;
    }
    game_->step(in, dt);
    if (game_->isOver()) {
        overFrames_ = 0;
        s_.sfx(game_->result() ? "score" : "gameover");
    }
}

void GameScene::finish() {
    const auto& info = game_->info();
    const auto result = game_->result();
    const auto key = game_->scoreTableKey();
    if (result && s_.scores.table(key, info.order).qualifies(*result)) {
        s_.scenes.replace<ScoreEntryScene>(s_, key, info.order, *result, info.title);
    } else {
        s_.scenes.pop();
    }
    overFrames_ = -1;
}

void GameScene::draw(gfx::Renderer& r) const {
    game_->render(r);
    if (overFrames_ < 0) return;
    const bool won = game_->result().has_value() && game_->info().order == io::ScoreOrder::LowerIsBetter;
    r.fill(Rect(24, 54, 116, 38), Shade::Dark);
    r.fill(Rect(22, 52, 116, 38), Shade::Paper);
    r.rect(Rect(22, 52, 116, 38), Shade::Ink);
    r.text(won ? "CLEARED!" : "GAME OVER", {80, 58}, TextStyle{}.align(Align::Center));
    const auto line = won ? fmt("TIME %us", static_cast<unsigned>(*game_->result())) : fmt("SCORE %u", static_cast<unsigned>(game_->score()));
    r.text(line.data(), {80, 72}, TextStyle{}.align(Align::Center));
}

void GameScene::onHidden() {
    // Auto-pause, and keep a save state in case the tab never comes back.
    if (overFrames_ >= 0 || !game_->isPlaying()) return;
    if (game_->supportsSaveState()) {
        try {
            saveGame(*game_, s_.persistence);
        } catch (const SaveError& e) {
            logWarn("autosave failed: ", e.what());
        }
    }
    if (!dynamic_cast<PauseScene*>(s_.scenes.top())) s_.scenes.push<PauseScene>(s_, *this);
}

void GameScene::restart() {
    deleteSave(game_->info().id, s_.persistence);
    game_->reset();
}

void GameScene::saveAndQuit() {
    try {
        saveGame(*game_, s_.persistence);
        s_.toast("GAME SAVED");
    } catch (const SaveError& e) {
        s_.toast("SAVE FAILED");
        logWarn("save failed: ", e.what(), " ", e.context());
    }
    quit();
}

void GameScene::quit() {
    s_.scenes.pop();   // pause
    s_.scenes.pop();   // this
}

} // namespace nb::scenes
