#include "TestHarness.hpp"

#include "device/DeviceShell.hpp"
#include "input/InputManager.hpp"

using namespace nb;
using namespace nb::input;

namespace {

const gfx::BitmapFont& mini() {
    static const auto f = gfx::BitmapFont::load(std::filesystem::path(NB_ASSET_DIR) / "fonts/mini3x5.fnt");
    return f;
}

// Landscape 320×180 at scale 1, so physical == surface coordinates.
device::DeviceShell landscape() {
    device::DeviceShell s(mini());
    s.configure(std::make_unique<device::LandscapeLayout>(), {320, 180});
    return s;
}

} // namespace

TEST("Edge detection across frames") {
    InputState s;
    s.press(Button::A);
    CHECK(s.pressed(Button::A) && s.held(Button::A));
    s.endStep();
    CHECK(!s.pressed(Button::A) && s.held(Button::A));
    s.release(Button::A);
    CHECK(s.released(Button::A) && !s.held(Button::A));
}

TEST("Viewport integer scale and centring") {
    device::LandscapeLayout l;
    const auto v = device::fit(l, {1280, 720});
    CHECK(v.scale == 4 && v.surface == (Vec2i{320, 180}) && v.offset == (Vec2i{0, 0}));
    device::PortraitLayout p;
    const auto pv = device::fit(p, {1170, 2532});
    CHECK(pv.scale == 6 && pv.surface == (Vec2i{180, 400}));
    CHECK(pv.offset.x == (1170 - 1080) / 2);
}

TEST("Hit testing: LCD, D-pad sectors and dead zone, A/B, pills") {
    auto shell = landscape();
    const auto lcd = shell.hit({80 + 5, 18 + 7});
    CHECK(lcd.lcd && *lcd.lcd == (Vec2i{5, 7}));
    const Vec2i d{40, 100};
    CHECK(shell.hit(d + Vec2i{0, -15}).button == Button::Up);
    CHECK(shell.hit(d + Vec2i{15, 3}).button == Button::Right);
    CHECK(shell.hit(d + Vec2i{-15, -3}).button == Button::Left);
    CHECK(shell.hit(d + Vec2i{2, 14}).button == Button::Down);
    CHECK(shell.hit(d + Vec2i{1, 1}).dpadDeadZone);
    CHECK(shell.hit({293, 92}).button == Button::A);
    CHECK(shell.hit({267, 108}).button == Button::B);
    CHECK(shell.hit({160 - 14, 170}).button == Button::Select);
    CHECK(shell.hit({160 + 22, 170}).button == Button::Start);
    CHECK(!shell.hit({5, 5}).button);
}

TEST("Touch: two thumbs, sliding, dead zone keeps direction") {
    auto shell = landscape();
    InputManager in(shell);
    in.handle(FingerEvent{1, PointerPhase::Down, {40, 85}});   // Up
    in.handle(FingerEvent{2, PointerPhase::Down, {293, 92}});  // A
    in.beginFrame();
    CHECK(in.state().held(Button::Up) && in.state().held(Button::A));
    in.handle(FingerEvent{1, PointerPhase::Move, {41, 101}});  // dead zone
    in.beginFrame();
    CHECK(in.state().held(Button::Up));
    in.handle(FingerEvent{1, PointerPhase::Move, {55, 100}});  // roll to Right
    in.beginFrame();
    CHECK(in.state().held(Button::Right) && !in.state().held(Button::Up));
    CHECK(in.state().pressed(Button::Right));
    in.handle(FingerEvent{2, PointerPhase::Up, {293, 92}});
    in.beginFrame();
    CHECK(in.state().released(Button::A));
}

TEST("Keyboard and mouse sources") {
    auto shell = landscape();
    InputManager in(shell);
    in.handle(KeyEvent{Key::X, true, false});
    in.handle(KeyEvent{Key::Enter, true, false});
    in.beginFrame();
    CHECK(in.state().held(Button::A) && in.state().held(Button::Start));
    in.handle(KeyEvent{Key::X, false, false});
    in.handle(MouseEvent{PointerPhase::Down, {267, 108}});
    in.beginFrame();
    CHECK(!in.state().held(Button::A) && in.state().held(Button::B));
}

TEST("Gestures: tap fires on press, swipe, long-press") {
    auto shell = landscape();
    InputManager in(shell);
    in.handle(MouseEvent{PointerPhase::Down, {100, 50}});
    in.beginFrame();
    CHECK(in.state().lcdTap() && *in.state().lcdTap() == (Vec2i{20, 32}));
    in.handle(MouseEvent{PointerPhase::Move, {116, 52}});
    in.beginFrame();
    CHECK(in.state().gesture() && in.state().gesture()->kind == Gesture::Kind::SwipeRight);
    in.handle(MouseEvent{PointerPhase::Up, {116, 52}});
    in.beginFrame();
    in.handle(MouseEvent{PointerPhase::Down, {100, 50}});
    std::optional<Gesture> got;
    for (int i = 0; i < 30 && !got; ++i) {
        in.beginFrame();
        got = in.state().gesture();
    }
    CHECK(got && got->kind == Gesture::Kind::LongPress);
}
