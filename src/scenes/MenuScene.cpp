#include "scenes/MenuScene.hpp"

#include "games/GameRegistry.hpp"
#include "scenes/Common.hpp"
#include "scenes/GameScene.hpp"
#include "scenes/SaveState.hpp"
#include "scenes/SettingsScene.hpp"

#include <algorithm>
#include <numeric>

namespace nb::scenes {

using gfx::Align;
using gfx::Shade;
using gfx::TextStyle;
using input::Button;

MenuScene::MenuScene(Services& s) : s_(s), icons_(s.assets.sprites("sprites/icons.spr")) {
    order_.resize(s_.registry.catalog().size());
    std::iota(order_.begin(), order_.end(), 0);
}

const std::string& MenuScene::currentId() const { return s_.registry.catalog()[static_cast<std::size_t>(order_.front())].id; }

void MenuScene::refresh() {
    // Best score per cartridge across all its modes, refreshed whenever the menu comes back.
    best_.assign(s_.registry.catalog().size(), std::nullopt);
    for (std::size_t i = 0; i < best_.size(); ++i) best_[i] = s_.scores.bestFor(s_.registry.catalog()[i].id, s_.registry.catalog()[i].order);
    insertFrames_ = -1;
    prompt_.reset();
}

void MenuScene::start(bool resume) {
    resume_ = resume;
    insertFrames_ = 0;
    s_.sfx("insert");
}

void MenuScene::update(const input::InputState& in, Seconds) {
    ++frames_;
    if (insertFrames_ >= 0) {
        if (++insertFrames_ == 28) s_.scenes.push<GameScene>(s_, currentId(), resume_);
        return;
    }
    if (prompt_) {
        prompt_->tick();
        if (prompt_->handle(in)) s_.sfx("move");
        else if (in.pressed(Button::A) || in.pressed(Button::Start)) start(prompt_->cursor() == 0);
        else if (in.pressed(Button::B)) {
            prompt_.reset();
            s_.sfx("back");
        }
        return;
    }
    auto swiped = [&](input::Gesture::Kind k) { return in.gesture() && in.gesture()->kind == k; };
    if (in.pressed(Button::Right) || swiped(input::Gesture::Kind::SwipeLeft)) {
        std::rotate(order_.begin(), order_.begin() + 1, order_.end());
        s_.sfx("move");
    } else if (in.pressed(Button::Left) || swiped(input::Gesture::Kind::SwipeRight)) {
        std::rotate(order_.rbegin(), order_.rbegin() + 1, order_.rend());
        s_.sfx("move");
    } else if (in.pressed(Button::A) || in.pressed(Button::Start) ||
               (in.lcdTap() && Rect(40, 18, 80, 80).contains(*in.lcdTap()))) {
        if (hasSave(currentId())) {
            prompt_.emplace(Rect(36, 72, 100, 30), std::vector<std::string>{"CONTINUE", "NEW GAME"});
            s_.sfx("select");
        } else {
            start(false);
        }
    } else if (in.pressed(Button::Select)) {
        s_.sfx("select");
        s_.scenes.push<SettingsScene>(s_);
    }
}

void MenuScene::draw(gfx::Renderer& r) const {
    r.clear(Shade::Paper);
    titleBar(r, "CARTRIDGES");
    const auto idx = static_cast<std::size_t>(order_.front());
    const auto& info = s_.registry.catalog()[idx];

    // The cartridge: slides down into the slot once inserted.
    const int slide = insertFrames_ >= 0 ? std::min(insertFrames_, 20) * 3 : 0;
    const Rect cart(40, 18 + slide, 80, 80);
    r.fill(Rect(cart.x + 2, cart.y + 2, cart.w, cart.h), Shade::Dark);
    r.fill(cart, Shade::Light);
    r.rect(cart, Shade::Ink);
    r.fill(Rect(cart.right() - 8, cart.y, 8, 8), Shade::Paper);   // clipped corner
    r.line({cart.right() - 8, cart.y}, {cart.right() - 1, cart.y + 7}, Shade::Ink);
    for (int i = 0; i < 5; ++i) r.line({cart.x + 6 + i * 3, cart.y + 3}, {cart.x + 6 + i * 3, cart.y + 8}, Shade::Dark);
    const Rect label(cart.x + 6, cart.y + 14, 68, 60);
    r.fill(label, Shade::Paper);
    r.rect(label, Shade::Ink);
    r.draw(*icons_, icons_->indexOf(info.id), {label.x + 26, label.y + 8});
    r.text(info.title, {80, label.y + 32}, TextStyle{}.align(Align::Center));
    r.text(fmt("NB-%02u", static_cast<unsigned>(idx + 1)).data(), {80, label.y + 46}, TextStyle{}.font(gfx::Font::Mini).shade(Shade::Dark).align(Align::Center));

    // Slot lip covers the bottom of the sliding cartridge.
    r.fill(Rect(0, 100, 160, 44), Shade::Paper);
    r.fill(Rect(30, 99, 100, 3), Shade::Ink);

    if (insertFrames_ >= 0) return;
    if ((frames_ / 20) % 2 == 0) {
        r.text("<", {22, 52});
        r.text(">", {133, 52});
    }
    r.text(info.blurb, {80, 108}, TextStyle{}.font(gfx::Font::Mini).shade(Shade::Dark).align(Align::Center));
    if (const auto& b = best_[idx]) {
        const auto line = info.order == io::ScoreOrder::LowerIsBetter ? fmt("BEST %us", *b) : fmt("BEST %u", *b);
        r.text(line.data(), {80, 120}, TextStyle{}.align(Align::Center));
    } else {
        r.text("NO SCORE YET", {80, 120}, TextStyle{}.shade(Shade::Dark).align(Align::Center));
    }
    footer(r, "A: PLAY   SELECT: SETTINGS");

    if (prompt_) {
        r.fill(Rect(30, 64, 104, 42), Shade::Dark);
        r.fill(Rect(28, 62, 104, 42), Shade::Paper);
        r.rect(Rect(28, 62, 104, 42), Shade::Ink);
        r.text("SAVED GAME", {80, 65}, TextStyle{}.font(gfx::Font::Mini).align(Align::Center));
        prompt_->draw(r);
    }
}

} // namespace nb::scenes
