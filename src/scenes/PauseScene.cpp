#include "scenes/PauseScene.hpp"

#include "scenes/GameScene.hpp"

namespace nb::scenes {
using input::Button;

namespace {
enum Item { Resume, Restart, SaveQuit, Quit };
}

PauseScene::PauseScene(Services& s, GameScene& host)
    : s_(s), host_(host), list_(Rect(30, 54, 100, 52), {"RESUME", "RESTART", "SAVE & QUIT", "QUIT"}) {
    list_.setDisabled(SaveQuit, !host_.game().supportsSaveState());
}

void PauseScene::update(const input::InputState& in, double) {
    list_.tick();
    if (list_.handle(in)) {
        s_.sfx("move");
        return;
    }
    if (in.pressed(Button::Start) || in.pressed(Button::B)) {
        s_.sfx("back");
        s_.scenes.pop();
        return;
    }
    if (!in.pressed(Button::A)) return;
    switch (list_.cursor()) {
    case Resume: s_.sfx("select"); s_.scenes.pop(); break;
    case Restart: s_.sfx("select"); host_.restart(); s_.scenes.pop(); break;
    case SaveQuit:
        if (!host_.game().supportsSaveState()) { s_.sfx("error"); break; }
        s_.sfx("select");
        host_.saveAndQuit();
        break;
    case Quit: s_.sfx("back"); host_.quit(); break;
    default: break;
    }
}

void PauseScene::draw(gfx::Renderer& r) const {
    r.fill(Rect(0, 0, 160, 144), gfx::Dither::Half, gfx::Shade::Clear, gfx::Shade::Light);
    const Rect box(24, 34, 112, 76);
    r.fill(Rect(box.x + 2, box.y + 2, box.w, box.h), gfx::Shade::Dark);
    r.fill(box, gfx::Shade::Paper);
    r.rect(box, gfx::Shade::Ink);
    r.text("PAUSED", {56, 39});
    list_.draw(r);
}

}
