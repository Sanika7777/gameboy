# CLAUDE.md — NEOBRICK

NEOBRICK is a neo-retro handheld console that runs in a phone browser. One virtual device with a
pixelated 4-shade LCD, an on-screen D-pad, A/B and START/SELECT buttons, and five games:

| Cartridge    | Genre                      |
|--------------|----------------------------|
| SERPENT      | snake (classic phone style)|
| MINEFIELD    | mine sweeping              |
| STACKFALL    | falling-block puzzle       |
| FLAPWING     | tap-to-fly                 |
| SILVERBALL   | pinball                    |

It is also an object-oriented programming showcase, but only where a feature genuinely earns its
place: `docs/oop-map.md` records where. Never add a class, interface or pattern just to tick a
concept box. Lean code beats a complete checklist (see "Keep it lean" below).

**Read before writing code:** `design.md` (architecture, class design, file formats, game specs)
and `phases.md` (build order and exit criteria). They are the source of truth.

---

## Non-negotiables

1. **All logic is C++20.** Engine, renderer, UI, input, audio synthesis, file I/O and every game.
2. **The only non-C++ files allowed:**
   - `CMakeLists.txt`, `CMakePresets.json`
   - `web/shell.html` — viewport meta tag, full-screen canvas CSS and Emscripten's `{{{ SCRIPT }}}`
     placeholder. No game logic and no hand-written JavaScript. Keep it under ~40 lines.
     Sole exception: the one-line `var Module = { canvas: ... }` config, because SDL2 hard-codes
     the `#canvas` selector and Emscripten resolves it only through `Module.canvas`.
   - Data files in `assets/` (text formats defined in `design.md` §10), optional `web/manifest.json`.
   - Markdown docs.
   - **No `EM_ASM` / `EM_JS` blocks.** Use Emscripten's C APIs (`<emscripten.h>`,
     `<emscripten/html5.h>`). If something truly cannot be done without JavaScript, stop and ask.
3. **Third-party code: SDL2 only**, as a thin platform layer (window/canvas, texture upload, input
   events, audio device, timer). No SDL_image/ttf/mixer, no engines (raylib, SFML…), no JS
   frameworks, no external test library. Fonts, sprites, sound, physics and UI are written here.
4. **Pixel rule.** Games draw only into the 160×144 LCD buffer of shade indices 0–3. Integer screen
   coordinates, no anti-aliasing, nearest-neighbour integer scaling only.
5. **Original branding.** Do not use "Game Boy", "Tetris", "Flappy Bird", "Nokia" (or their logos,
   sprites, fonts, sounds) in UI text, asset names or identifiers. Mechanics are fine; branding isn't.

---

## Toolchain

- **emsdk** (latest), **CMake ≥ 3.24**, **Ninja**
- Native: Clang ≥ 16 or GCC ≥ 12, plus SDL2 dev package (`libsdl2-dev`, `brew install sdl2`, or vcpkg)
- Web: SDL2 comes from Emscripten's port (`-sUSE_SDL=2`). Do not switch to SDL3 — its Emscripten
  port is still marked experimental.

## Commands

```bash
# one-time
git clone https://github.com/emscripten-core/emsdk && cd emsdk
./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh            # every new shell (sets $EMSDK)

# native: fast iteration, debugger, tests
cmake --preset native-debug
cmake --build --preset native-debug
./build/native-debug/neobrick
ctest --preset native-debug --output-on-failure

# web: the product
cmake --preset web-release       # preset points CMAKE_TOOLCHAIN_FILE at $EMSDK's Emscripten.cmake
cmake --build --preset web-release
emrun --no_browser --hostname 0.0.0.0 --port 8080 build/web-release/index.html
# phone on the same Wi-Fi: open http://<laptop-ip>:8080/index.html
```

### Emscripten flags (set in CMakeLists.txt for the web presets)

```
compile:  -sUSE_SDL=2 -fwasm-exceptions
link:     -sUSE_SDL=2 -fwasm-exceptions -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web
          --preload-file ${CMAKE_SOURCE_DIR}/assets@/assets
          --shell-file   ${CMAKE_SOURCE_DIR}/web/shell.html
debug:    -O0 -sASSERTIONS=1 -gsource-map
release:  -O2
output:   index.html (+ index.js, index.wasm, index.data — all generated)
```

- `-fwasm-exceptions` must be on **both compile and link**. Exception catching is off by default in
  Emscripten; missing it at link time makes `catch` blocks silently not work.
- **No `-pthread`.** Threads need cross-origin isolation headers that most static hosts can't send.
- If the linker reports undefined `emscripten_idb_async_*`, add `-lidbstore.js` to link options.

Native warnings: `-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Woverloaded-virtual`.
Keep both builds warning-free.

---

## Repository layout

```
neobrick/
├── CLAUDE.md  design.md  phases.md
├── docs/oop-map.md            # concept → file:symbol → why (kept current)
├── CMakeLists.txt  CMakePresets.json
├── web/shell.html             # the one HTML file
├── assets/                    # read with std::ifstream; preloaded to /assets in the web build
│   ├── fonts/   (pixel5x7.fnt, mini3x5.fnt)
│   ├── sprites/ (*.spr)
│   ├── tables/  (silverball_*.tbl)
│   ├── levels/  (serpent_mazes.txt)
│   ├── data/    (palettes.txt, stackfall.txt)
│   └── audio/   (sfx.txt, *.trk)
├── src/
│   ├── main.cpp
│   ├── platform/   # ONLY place allowed to include SDL or emscripten headers
│   ├── core/       # App, Clock, Vec2, Rect, Random, Log, Error, Signal, RingBuffer
│   ├── gfx/        # PixelBuffer, Palette, Renderer, BitmapFont, Sprite, Compositor, effects
│   ├── input/      # Button, InputState, IInputSource + sources, GestureRecognizer
│   ├── audio/      # Channel hierarchy, Envelope, Mixer, SfxLibrary, MusicPlayer
│   ├── io/         # Paths, TextReader, IniFile, BinaryReader/Writer, Serializer, HighScoreTable, persistence
│   ├── device/     # DeviceShell, ILayout, PortraitLayout, LandscapeLayout
│   ├── ui/         # Widget, ListView, Toast
│   ├── scenes/     # Scene, SceneStack, Boot/Menu/Game/Pause/ScoreEntry/Settings/Error scenes
│   └── games/
│       ├── Game.*  GridGame.*  PhysicsGame.*  GameObject.*  GameRegistry.*
│       └── serpent/  minefield/  stackfall/  flapwing/  silverball/
└── tests/
    ├── TestHarness.hpp
    └── test_*.cpp
```

---

## Architecture rules (details in design.md)

- **Layers depend downward only:** games → scenes/ui/device → gfx/input/audio/io → core → platform.
  Games never include SDL or Emscripten headers.
- `#ifdef __EMSCRIPTEN__` appears only in `src/platform/` and `main.cpp`.
- **Fixed 60 Hz simulation.** Games get a constant `dt = 1/60 s`. Physics games substep internally.
- **Randomness** only through an injected, seedable `nb::Random` (wraps `std::mt19937`). Never `rand()`.
- **No heap allocation per frame** in steady state. Reserve containers up front.
- **The `App` object must not live on `main()`'s stack in web builds.** `emscripten_set_main_loop_arg`
  with `simulate_infinite_loop = true` unwinds `main`; keep the App in a `static std::unique_ptr`.
- **No exception may escape the frame callback.** `App::frame()` catches and routes to `ErrorScene`.

## Code style

- Namespace `nb`, with nested namespaces per area: `nb::gfx`, `nb::io`, `nb::games::serpent`.
- `PascalCase.hpp/.cpp`, one main class per file, `#pragma once`.
- Types `PascalCase`; functions and variables `camelCase`; data members `trailing_`;
  constants `kPascalCase`; enums are `enum class`.
- `override` on every override, `final` on leaf classes, virtual destructor on every polymorphic base.
- **Rule of Zero by default.** `gfx::PixelBuffer` is the single deliberate Rule-of-Five class with
  raw ownership. Do not add others. No raw `new`/`delete` anywhere else — use `std::make_unique`.
- Const-correct. `[[nodiscard]]` on functions whose result must be used. `noexcept` on destructors,
  move operations, `swap`, and the audio callback.
- Default to `std::array` / `std::vector`. When another container is used, record why in the
  containers table of `docs/oop-map.md`.
- **No comments in source files** — no `// Role:` / `// Demonstrates:` headers, no docstrings.
  Names carry the meaning; `docs/oop-map.md` and `design.md` carry the explanations.

## Keep it lean (ponytail rules)

Before writing anything, stop at the first rung that holds:
1. Does it need to exist at all? Speculative need → don't write it.
2. Is it already in this codebase? Reuse it.
3. Does the standard library do it? Use it (`std::clamp`, `std::ranges::find`, …).
4. Can it be one line? Make it one line. Only then write the minimum code that works.

Concretely:
- No interface with one implementation, no factory with one product, no config nobody sets.
- No wrapper or strong type that every caller immediately unwraps (e.g. `dt` is a plain `double`).
- No mixin or helper that duplicates something that already exists (one instance counter: `Game::liveInstances`).
- No caches for data loaded once: load assets directly with `SpriteSheet::load(io::Paths::asset(...))`.
- Write `= delete` copy operations in the class itself; no base class just to forbid copying.
- Delete dead code (unused members, enum values, presets) as soon as you notice it.
- Fewest files possible; deletion over addition. Bug fixes go to the root cause, once.

## Exceptions policy

- Recoverable failures throw subclasses of `nb::Error` (`src/core/Error.hpp`): missing or malformed
  assets, corrupt saves, platform init failures. API misuse throws `nb::ContractViolation`.
- Normal gameplay outcomes are never exceptions (game over, invalid move, failed rotation).
- No throwing inside per-frame update/draw paths for expected conditions.
- Catch by `const&` at boundaries only: `App::frame`, `GameScene` start-up, save loading. Add context
  with `std::throw_with_nested`; `ErrorScene` unwinds the chain.
- `what()` messages are shown on a 26-column pixel screen: short headline, details in context fields.

## File handling policy

- All file access goes through `src/io/` using `<fstream>` / `<filesystem>`. No C stdio.
- Read-only data comes from `/assets/...`. Writable files live under `io::Paths::saveDir()`.
- Binary formats are written field by field, little-endian, with magic, version and CRC32.
  Never `write(reinterpret_cast<const char*>(&someStruct), sizeof someStruct)`.
- Writes go to `<name>.tmp`, then `std::filesystem::rename` over the real file. After a successful
  write call `persistence.commit(path)` so the web build mirrors it to IndexedDB.
- Every text parser reports file name and line number on failure (`nb::ParseError`).

## Testing

- In-house harness `tests/TestHarness.hpp`. Tests run natively via `ctest`.
- Game logic must run headless: construct a game with a seeded `Random` and scripted `InputState`,
  step N frames, assert on state. Renderer tests hash the LCD buffer against known values.
- Every parser and serializer has a round-trip test and a corrupt-input test that expects the exact
  exception type.

---

## Current priority: laptop first (2026-10-23)

Make everything work in the **laptop browser on localhost with keyboard and mouse** first. Phone
checks (touch, Android Chrome, iOS Safari) are deferred to one later pass and do not block phases.
Verify each change with the web build in Chrome (serve `build/web-release` on `localhost:8080`).
The native app needs `libsdl2-dev` (install with `sudo apt-get install libsdl2-dev`); native tests
build without it.

## How to work on this repo (Claude)

1. Work **phase by phase** from `phases.md`. Don't start a phase until the previous phase's exit
   criteria pass. Tick checkboxes in `phases.md` as tasks complete.
2. Before creating a class, check `design.md` for its name and responsibility. If you need to
   deviate, update `design.md` in the same change and say why.
3. When code demonstrates an OOP concept, add or update its row in `docs/oop-map.md`
   (concept → `file:symbol` → one-line explanation).
4. After each task: build native and web, run tests. Small commits: `phase-N: <what changed>`.
5. **Ask first** before: adding a dependency, adding any non-C++ file, bumping a save-file format
   version, changing the LCD resolution, the 4-shade rule, or the button set.
6. You cannot test on a real phone. End each phase with a short checklist of what the human should
   verify on Android Chrome and iOS Safari (collected for the deferred phone pass, see above).

## Definition of done (every feature)

- Builds warning-free on native and web; all tests pass.
- Works with touch (portrait and landscape) and keyboard.
- No per-frame allocations; holds 60 fps.
- `docs/oop-map.md` and `phases.md` updated.
