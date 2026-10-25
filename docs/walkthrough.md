# Walkthrough: one tap, from the screen to a pixel

Follow a single tap on FLAPWING's LCD (or a press of X on the keyboard) until the bird's new
position is on screen. File references are under `src/`.

1. **The browser event.** The finger touches the canvas. Emscripten's SDL2 port turns the DOM
   `touchstart` into an `SDL_FINGERDOWN` with coordinates normalised to 0..1.

2. **Platform → `PlatformEvent`.** `platform/SdlPlatform.cpp:SdlPlatform::pumpEvents` multiplies by
   the canvas backing-store size (CSS size × devicePixelRatio, see design §4.4) and emits
   `FingerEvent{id, Down, phys}`. It ignores SDL's synthetic mouse events for touches
   (`which == SDL_TOUCH_MOUSEID`) so the tap isn't counted twice.

3. **App dispatch.** `core/App.cpp:App::handle` uses `std::visit` with the `overloaded` lambdas;
   finger events go to `input::InputManager::handle`, which hands them to every `IInputSource`.

4. **Hit testing.** `input/InputSources.cpp:TouchSource::handle` asks
   `device/DeviceShell.cpp:DeviceShell::hit` what is under the point: it converts physical pixels to
   surface pixels (`(phys − offset) / scale`) and finds the LCD rectangle, so the result is
   `Hit{lcd = {x, y}}`. The finger is remembered by id; the LCD press goes to
   `GestureRecognizer::down`, which records a tap immediately (no waiting for release).

5. **Frame sampling.** `App::frame` asks `Clock::advance` how many fixed 1/60 s steps are due. If at
   least one is, `InputManager::beginFrame` ORs every source's buttons into a `ButtonSet` and builds
   the frame's `InputState` (held/pressed/released + `lcdTap`).

6. **Scenes → game.** `scenes::SceneStack::update` calls the top scene, `GameScene::update`, which
   calls `Game::step` — the Template Method: `handleInput`, then `updateWorld`.

7. **Game logic.** `games/flapwing/FlapwingGame.cpp:FlapwingGame::handleInput` sees `in.lcdTap()`,
   calls `Bird::flap()` (velocity −170 px/s) and emits `playSfx("flap")` on the `EventBus`.
   `PhysicsGame::updateWorld` (final) runs two substeps of `simulate`, moving the bird and pipes.

8. **Sound, in parallel.** The `EventBus` signal reaches the App's slot, which looks up `"flap"` in
   the `SfxLibrary` and pushes an `AudioCommand` into the lock-free `RingBuffer`. On the audio
   thread, `Mixer::render` pops it and starts a `PulseChannel` with a rising sweep.

9. **Drawing.** `SceneStack::draw` → `Game::render` → `FlapwingGame::drawWorld` draws clouds, hills,
   pipes, the bird sprite and the ground through `gfx::Renderer` into the 160×144 `PixelBuffer` of
   shade indices 0–3. `drawHud` adds the score.

10. **Composition.** `gfx/Compositor.cpp:Compositor::compose` maps every shade through the
    selected `Palette` with `std::transform`, applies the `LcdEffect` strategy (e.g. ghosting
    blends 35 % of last frame) and copies the 160×144 colours into the device face that
    `DeviceShell::beginFrame` prepared (the case is cached and only redrawn when a button changes).

11. **Present.** `SdlPlatform::present` uploads the face to one streaming texture and draws it with
    an integer scale and centred offset, nearest-neighbour, so every LCD pixel is a perfect square.
    The browser shows the bird one flap higher — on the same frame as the tap.
