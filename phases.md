# NEOBRICK — Build Phases

Work top to bottom. A phase is finished only when its **exit criteria** pass on both the native and
web builds and the human has done the **phone check**. Tick boxes as tasks complete.

> **Current priority (2026-10-23): laptop first.** Every phase is built and verified in the laptop
> browser on localhost with keyboard and mouse (headless Chrome driven over DevTools: boot, menu,
> all five games, pause, settings, score entry, save/continue across reloads, error screen).
> Phone checks are deferred to one later pass. The native app also waits on `libsdl2-dev`
> (needs `sudo`); native *tests* build and pass without it.
Section references (§) point to `design.md`.

## At a glance

| #  | Phase                          | Size | Delivers                                        | First OOP concepts                          |
|----|--------------------------------|------|-------------------------------------------------|---------------------------------------------|
| 0  | Toolchain & first pixel        | S    | Crisp checkerboard on a phone, tap works        | RAII deleters, `= delete`, private inheritance |
| 1  | Core & pixel renderer          | M    | Primitives, text, palettes, fixed timestep      | Rule of Five, templates, concepts, overloading |
| 2  | Device shell & input           | M    | The console face, touch + keyboard input        | Interfaces, `variant`/`visit`, Strategy     |
| 3  | Files, assets & errors         | L    | Assets from files, settings, high scores, web saves | Exception hierarchy, file I/O, specialization |
| 4  | Scenes, menu & game framework  | L    | Boot → menu → game → pause → scores loop        | Abstract classes, Observer, Factory, nested exceptions |
| 5  | Audio & feedback               | M    | Chip synth, sound effects, haptics              | Polymorphic channels, lock-free ring buffer |
| 6  | SERPENT                        | M    | First complete game (**MVP**)                   | Multilevel inheritance, CRTP, custom hash   |
| 7  | MINEFIELD                      | M    | Second game, direct touch on cells              | `set`, BFS `queue`, score-order strategy    |
| 8  | STACKFALL                      | L    | Falling-block game with good feel               | `array` tables, 7-bag `shuffle`             |
| 9  | FLAPWING                       | S    | Physics game base, one-tap game                 | Multiple inheritance, virtual (diamond)     |
| 10 | SILVERBALL                     | L    | Pinball from a table file                       | `Collider` polymorphism, file-driven factory |
| 11 | Save states & polish           | M    | Suspend/resume, LCD effects, themes, accessibility | Binary serialization, Strategy           |
| 12 | Mobile hardening & deploy      | M    | Public URL, tested on real devices              | —                                           |
| 13 | OOP audit & documentation      | S    | Complete concept map, diagrams, viva notes      | —                                           |

**MVP = Phases 0–6:** one game fully playable on a phone with saves and sound. Everything after adds
games and polish on a proven base.

## Definition of done (every phase)

- [x] Native and web builds are warning-free; `ctest` passes
      *Web, native library and tests: zero warnings, 59/59 tests. Native app binary waits on `libsdl2-dev`.*
- [x] Works with touch (portrait **and** landscape) and keyboard
      *Keyboard and mouse verified in the browser; touch and portrait are unit-tested, phone pass deferred.*
- [x] No per-frame allocations; 60 fps
      *Draw paths format into fixed buffers. Event-driven allocations remain where the design asks for
      node containers: SERPENT's `unordered_set` per move, SILVERBALL sparks (`std::list`) per hit.*
- [x] `docs/oop-map.md` updated for every concept introduced
- [ ] Phone check done on Android Chrome and iOS Safari
      *Deferred: laptop-first. See the phone checklist in each phase.*

---

## Phase 0 — Toolchain & first pixel on the phone · S

**Goal:** prove the whole pipeline (C++ → WebAssembly → phone browser → touch) before writing
the engine.

- [ ] Install emsdk, CMake, Ninja, SDL2 dev package (see `CLAUDE.md`)
      *emsdk, CMake, Ninja done; `libsdl2-dev` still needs `sudo apt-get install libsdl2-dev`*
- [x] `CMakeLists.txt` + `CMakePresets.json` with `native-debug`, `native-release`, `web-debug`,
      `web-release`; Emscripten flags exactly as in `CLAUDE.md`
- [x] `web/shell.html`: viewport meta (`width=device-width, initial-scale=1, viewport-fit=cover,
      user-scalable=no`), body margin 0 and no scroll, canvas `id="canvas"` filling the viewport,
      `touch-action: none`, `image-rendering: pixelated`, no text selection / touch callout
- [x] `docs/oop-map.md` created with the table headings from design §13
- [x] `platform/SdlHandles.hpp`: `SdlWindow`, `SdlRenderer`, `SdlTexture` as `std::unique_ptr` with
      custom deleters
- [x] `core/NonCopyable`; `App : private NonCopyable`
- [x] `main.cpp`: App in a `static std::unique_ptr`; `emscripten_set_main_loop_arg` on web, `while`
      loop natively
- [x] Draw a 160×144 checkerboard in 4 shades into a streaming texture; nearest-neighbour, integer
      scale, centred
- [x] Tap/click anywhere inverts the screen
- [x] Throw and catch a test exception at start-up; log "exceptions OK" (proves `-fwasm-exceptions`)
- [ ] **Spike:** canvas backing-store size vs `devicePixelRatio` with SDL2 on a real phone; write
      the working call sequence into design §4.4
      *Sequence written into §4.4; headless Chrome at DPR 3 is pixel-exact in both orientations.
      Tick after the real-phone check.*

**Exit criteria**
- Checkerboard is perfectly sharp on phone and desktop (no blurry edges)
- Tap inverts the screen on the phone; "exceptions OK" appears in the browser console

**Phone check:** open over LAN, rotate the phone, confirm no page scroll or pinch-zoom.

---

## Phase 1 — Core & pixel renderer · M

**Goal:** everything needed to draw retro graphics and run a fixed-timestep loop.

- [x] `core/Vec2.hpp` — `Vec2<Arithmetic T>`, constexpr ctors, operators, `<=>`, friend `operator<<`
- [x] `core/Rect.hpp` — delegating ctor, `contains`, `operator&`
- [x] `core/Seconds.hpp` (explicit strong type), `core/Clock` (fixed 1/60 s step, delta clamp 0.25 s)
- [x] `core/Random` wrapping `std::mt19937` with seed control
- [x] function template `scenes::fmt` (was `core/Math.hpp`, removed as unused)
- [x] `core/Log` — Meyers singleton (with comment on trade-offs), variadic `write` with fold
      expression, native append-mode `neobrick.log`
- [x] `gfx/Shade` enum class; `gfx/Palette` (`initializer_list` ctor, static `preset()`)
- [x] `gfx/PixelBuffer` — **Rule of Five** with copy-and-swap and `noexcept` moves
- [x] `gfx/RgbaSurface` — Rule of Zero counterpart
- [x] `gfx/Renderer` — clear, pixel, line, rect, fill (+ dither overload), circle, fillCircle,
      sprite draw overloads, clipping, camera
- [x] `gfx/BitmapFont` with a few hard-coded glyphs (file loading arrives in Phase 3);
      *Fonts load from `.fnt` files directly (Phase 3's loader arrived early), so no hard-coded glyphs.*
      `TextStyle` fluent builder
- [x] `gfx/Compositor` — shade → RGBA via palette (`std::transform`), upload, present
- [x] `tests/TestHarness.hpp` + tests: Vec2/Rect maths, PixelBuffer copy/move/self-assignment,
      renderer hash tests

**Exit criteria**
- A test scene shows every primitive, dither, and text; renderer hash tests pass
- Moving a sprite at 1 px/step is smooth at 60 fps

**Phone check:** test scene sharp in both orientations; no stutter.

---

## Phase 2 — Device shell & input · M

**Goal:** the console face and complete, responsive controls.

- [x] `device/ILayout` + `PortraitLayout` + `LandscapeLayout` (Strategy); responsive surface size
      and integer scale per design §3.2–3.4
- [x] `device/DeviceShell` — case, bezel, LCD frame, power LED, wordmark, D-pad, A/B,
      SELECT/START, speaker slots; cached and redrawn only when dirty; pressed-state visuals
- [x] `core/PlatformEvent` variant + `overloaded` visitor helper
- [x] Platform resize/orientation and visibility handling → layout recompute, auto-pause flag
- [x] `input/Button`, `ButtonSet` (`std::bitset<8>`), `InputState` (held / pressed / released)
- [x] `input/IInputSource` + `KeyboardSource`, `TouchSource`, `MouseSource`; `InputManager` owning
      `std::vector<std::unique_ptr<IInputSource>>`
- [x] Touch: per-finger tracking, slide between buttons, angle-based D-pad with dead zone, enlarged
      *Implemented and unit-tested; real-phone verification deferred.*
      hit zones, ignore synthetic mouse events from touch
- [x] LCD tap mapping (`lcdTap`) and `GestureRecognizer` (swipe, long-press)
- [x] Haptic tick via platform `vibrate(ms)`
      *No-op on laptops; works on Android Chrome.*
- [x] Debug overlay (toggle with SELECT+START): shows held buttons and finger positions
- [x] Tests: hit-testing, D-pad angle sectors, edge detection across frames

**Exit criteria**
- All 8 buttons work by touch, mouse and keyboard; D-pad + A held together with two thumbs works
- Rotating the phone switches layout live, with no reload and no lost input

**Phone check:** two-thumb chords, rolling the thumb around the D-pad, left-handed layout mirror.

---

## Phase 3 — Files, assets & errors · L

**Goal:** all data comes from files; settings and high scores persist, including on the web.

- [x] `core/Error.hpp` — full hierarchy from design §9.1
- [x] `io/Paths` (`/assets`, `saveDir()`), `io/TextReader` (line numbers, tokenizing with
      `std::string_view`, throws `ParseError`)
- [x] Font loader (`.fnt`) and create `pixel5x7.fnt` + `mini3x5.fnt` (original glyphs)
- [x] Sprite loader (`.spr`) and `SpriteSheet`; shell sprites move to files
      *The console face is drawn procedurally (`DeviceShell`), so it has no sprites; game sprites are `.spr` files.*
- [x] `gfx/AssetCache` — `unordered_map` of `weak_ptr`, returns `shared_ptr<const T>`;
      function-try-block constructor for preloads
- [x] `assets/data/palettes.txt` loaded at start-up
- [x] `io/IniFile` with `get<T>` full specializations (`bool`, `int`, `std::string`); `Settings`
      built on it with strong-guarantee `apply()`
- [x] `io/BinaryWriter` / `BinaryReader` (`operator<<` / `>>`), `Serializer<T>` with full and
      partial specializations, `crc32()`
- [x] Atomic write helper: write `.tmp` → `std::filesystem::rename`
- [x] `io/HighScoreTable` — `ScoreEntry` with `<=>`, `ScoreOrder` strategy, `std::stable_sort`,
      strong-guarantee `submit`; `scores.dat` format per design §10.4
- [x] `io/IPersistence` + `NativePersistence` + `WebPersistence`
      (`emscripten_idb_async_load/store`) per design §10.8
- [x] Recovery behaviour from design §9.4 (`.bak`, defaults, Toast message queued for Phase 4)
- [x] Tests: round-trip for ini/scores/fonts/sprites; truncated file, bad magic, bad CRC, newer
      version, bad line → exact exception types; `HighScoreTable` ordering both directions

**Exit criteria**
- Changing a setting and setting a test high score survive a page reload on the phone and an app
  restart natively
- A deliberately corrupted `scores.dat` is backed up and replaced, and the app keeps running

**Phone check:** reload the page and confirm data persists; try a private/incognito tab and confirm
the app still runs (saves may not persist there).

---

## Phase 4 — Scenes, menu & game framework · L

**Goal:** the full app flow with a placeholder game, so real games just plug in.

- [x] `scenes/Scene` (abstract) and `SceneStack` (deferred ops, overlays, `push<T>(Args&&...)`)
- [x] `core/Signal<Args...>` + `Connection` (RAII, `weak_ptr` slots) and `EventBus`
- [x] `ui/Widget` Composite: `Panel`, `Label` (`mutable` width cache), `ListView`
      (`operator++` wrap), `Toast`
- [x] `games/Game` (abstract, Template Method, deleted copies, static `liveInstances_`),
      `GameContext`, `GameInfo`
- [x] `games/GameRegistry` (factory map of `std::function`); `UnknownGameError`
- [x] `games/DemoGame` — bouncing ball placeholder to exercise the framework
      *Skipped: the five real games were built in the same pass and exercise the framework; `GameRegistry` tests cover the rest.*
- [x] `BootScene` — logo slide, PRESS START, first-input hook for audio unlock (used in Phase 5)
- [x] `MenuScene` — cartridge carousel (`std::rotate`), best score per cartridge, SELECT → Settings
- [x] `GameScene` — hosts a `Game`, START → pause, game over → score entry
- [x] `PauseScene` overlay (Resume / Restart / Save & Quit (disabled until Phase 11) / Quit)
- [x] `ScoreEntryScene` — 3 initials
- [x] `SettingsScene` with sub-menus tracked by `std::stack`; palette, case, volume, haptics,
      layout, left-handed, reset data (with confirm)
- [x] `ErrorScene` — unwinds nested exceptions; `dynamic_cast<const ParseError*>` for file:line
- [x] `App::frame` try/catch routing to `ErrorScene`; palette fade transitions between scenes
- [x] Tests: scene stack deferral and overlay order, registry duplicates/unknown ids, signal
      disconnect on `Connection` destruction

**Exit criteria**
- Boot → Menu → Demo → Pause → Quit → Menu loops cleanly; score entry saves
- Renaming a sprite file shows a readable `ErrorScene`, and A returns to the menu
- `Game::liveInstances()` returns to 0 after leaving a game (no leaks)

**Phone check:** navigate everything by touch only; rotate mid-menu; background the tab and return.

---

## Phase 5 — Audio & feedback · M

**Goal:** retro sound with no crackle, plus haptics, all driven by events.

- [x] Platform audio device: 22,050 Hz, mono, 16-bit; opened/unpaused on first user input
- [x] `audio/Channel` (abstract) → `PulseChannel`, `WaveChannel`, `NoiseChannel`; `Envelope`
- [x] `audio/Mixer` (master volume 0–7, soft clip); callback `noexcept`, no allocation,
      `std::span<std::int16_t>` output block
- [x] `core/RingBuffer<T, N>` SPSC (`std::array` + `std::atomic`) for `AudioCommand`s
- [x] `assets/audio/sfx.txt` + `SfxLibrary` parser (menu move, select, back, boot chime, eat,
      line clear, flap, bumper, explode, game over…)
- [x] Wire `EventBus::playSfx` and `EventBus::vibrate` to audio and haptics; power LED reflects mute
- [x] Pause audio when the page is hidden
- [ ] *Stretch:* `MusicPlayer` + `.trk` format + one original menu theme
      *Skipped (stretch goal).*
- [x] Tests: ring buffer wrap/full/empty, envelope stages, sfx parser errors

**Exit criteria**
- Boot chime and menu sounds play on the phone after the first tap, without clicks or crackle
- Volume and mute settings apply immediately and persist

**Phone check:** sound with the silent switch off (iOS), lock and unlock the phone, background the tab.

---

## Phase 6 — SERPENT · M · (MVP complete)

**Goal:** the first real game; establishes the grid-game foundations. Spec: design §11.1.

- [x] `core/Grid<T>` — `operator()`, `at()` throwing `std::out_of_range`, `neighbours()`,
      `begin()/end()` iterators
- [x] `games/GridGame` (abstract, protected helpers) — multilevel: `Game → GridGame → SerpentGame`
- [x] `core/Counted<T>` (CRTP); `SerpentGame final : public GridGame, private Counted<SerpentGame>`
- [x] `std::hash<Vec2i>` specialization; occupancy `std::unordered_set<Vec2i>`
- [x] Body `std::deque<Vec2i>`; turn buffer `std::queue<Direction>` (max 2, no reversals)
- [x] Modes Box / Wrap / Maze; `assets/levels/serpent_mazes.txt` loader (`ParseError` on bad grids)
- [x] Speed levels 1–9 select screen; food, bonus critter with timer bar; scoring per spec
- [x] Original sprites (head with open-mouth frame, body, tail, food, critter)
- [x] Swipe control via `GestureRecognizer`
- [x] `GameTestAccess` friend class for tests
- [x] High scores per mode (`serpent.box`, `serpent.wrap`, `serpent.maze`)
- [x] Tests: grows on eat, dies on self/wall, wraps in Wrap mode, no 180° reversal, deterministic
      food placement with a fixed seed

**Exit criteria**
- Full game playable start to finish on the phone with D-pad and with swipes; high score saved

**Phone check:** play at level 9 — quick double turns must register; no missed swipes.

---

## Phase 7 — MINEFIELD · M

**Goal:** a turn-based grid game with direct touch on the LCD. Spec: design §11.2.

- [x] Difficulty select (Rookie / Veteran / Expert); Expert horizontal viewport scrolling
- [x] Mine placement after first reveal (`std::iota` → `std::shuffle`), safe 3×3
- [x] Flood reveal with `std::queue` BFS; adjacency via `std::count_if`
- [x] Flags in `std::set<Vec2i>`; chording
- [x] Controls: cursor + A/B; SELECT toggles tap mode; tap and long-press on cells via
      `screenToCell`
- [x] HUD: mines left, status face, timer; win/lose reveal animation
- [x] `ScoreOrder::LowerIsBetter` tables per difficulty
- [x] Tests: first reveal never a mine, flood fill stops at numbers, chording correctness, win
      detection

**Exit criteria**
- All three difficulties playable by touch alone and by keyboard; best times saved separately

**Phone check:** long-press flagging doesn't trigger browser text selection or a context menu.

---

## Phase 8 — STACKFALL · L

**Goal:** a falling-block game that feels tight. Spec: design §11.3.

- [x] Piece data: `static constexpr` rotation tables (`std::array`), wall-kick table
- [x] 7-bag randomizer with `std::shuffle`; next queue `std::deque`; hold
- [x] Gravity, soft/hard drop, ghost piece (dither), lock delay with move-reset limit
- [x] DAS/ARR for held ◀▶; values loaded from `assets/data/stackfall.txt`
- [x] Line detection with `std::all_of`, flash animation, collapse
- [x] Scoring and level progression per spec
- [x] Side panel: HOLD, NEXT ×3, SCORE, LEVEL, LINES
- [x] Tests: every rotation of every piece, wall kicks at edges, 7-bag contains each piece once per
      bag, scoring for 1–4 lines, top-out detection

**Exit criteria**
- Holding ◀ slides the piece smoothly to the wall; rotation near walls feels fair
- Plays to level 10+ without slowdown on the phone

**Phone check:** rapid tapping of A/B for rotation; holding ▼ while pressing ◀▶.

---

## Phase 9 — FLAPWING · S

**Goal:** the physics-game base plus the most phone-friendly game. Spec: design §11.4.

- [x] `games/Identifiable`, `IUpdatable`, `IDrawable` (both inherit `Identifiable` **virtually**),
      `GameObject : public IUpdatable, public IDrawable`
- [x] `games/PhysicsGame` (abstract; `updateWorld` is `final` and calls `simulate()` per substep);
      `Camera`
- [x] `Bird`, `PipePair` (recycled `std::deque`), `ParallaxLayer` ×3
- [x] AABB collision with forgiving hitbox; ground collision
- [x] Day/night palette remap; medals; score
- [x] Controls: LCD tap, A, ▲
- [x] Tests: gravity/flap bounds, pipe recycling keeps a constant count, passing a pipe scores once

**Exit criteria**
- Tap-to-flap responds on the same frame; difficulty ramps smoothly; high score saved

**Phone check:** fast repeated taps, tapping with the phone in landscape.

---

## Phase 10 — SILVERBALL · L

**Goal:** a data-driven pinball game. Spec: design §11.5.

- [x] `Ball`; `Collider` → `CircleCollider`, `SegmentCollider` → `FlipperCollider`;
      `std::optional<Contact>` results
- [x] `TableElement : GameObject` with virtual `onHit`; Wall, Bumper, Slingshot, DropTarget,
      Rollover, Flipper, Plunger, Drain
- [x] `TableLoader` — keyword → factory lambda `std::map`; `ParseError` with line numbers
- [x] `assets/tables/silverball_classic.tbl` (original layout)
- [x] 4 substeps per frame; flipper surface velocity transfer; bumper kicks
- [x] Vertical camera with smoothing over the 288-px table
- [x] Rules: 3 balls, ball save, multiplier from target groups, rollover bonus, end-of-ball bonus
      (`std::accumulate`); timers in `std::priority_queue`
- [x] Nudge and TILT
- [x] Spark particles in `std::list` with `remove_if` (document the trade-off in `oop-map.md`)
- [x] Touch: left/right LCD halves also flip
- [x] Tests: ball never passes through a wall at maximum speed, flipper hit adds speed, table
      parser rejects unknown keywords and missing attributes

**Exit criteria**
- A full 3-ball game is playable and fun on the phone with two thumbs
- A broken `.tbl` file shows a clear `ErrorScene` naming the file and line

**Phone check:** both flippers held at once, flipper + plunger, nudge spam until TILT.

---

## Phase 11 — Save states & polish · M

**Goal:** comfort features that make it feel finished.

- [x] `Game::save` / `load` for all five games (`<game>.sav` format, design §10.5); Pause →
      Save & Quit; Menu → CONTINUE?
- [x] Auto-save when the page is hidden mid-game
- [x] `LcdEffect` strategy: none / ghosting / pixel grid
- [x] All six LCD palettes and five case themes selectable; live preview in Settings
- [x] Transitions: cartridge insert animation, palette fades everywhere
- [x] Accessibility: Stark palette, reduce-flashing option, left-handed layout
- [ ] *Stretch:* menu music
      *Skipped (stretch goal).*
- [x] Tests: save/load round-trip for every game state; `.sav` with a newer version is rejected
      cleanly

**Exit criteria**
- Closing the tab mid-game and reopening offers CONTINUE and restores the exact state

**Phone check:** swipe the browser away mid-game, reopen, continue.

---

## Phase 12 — Mobile hardening & deploy · M

**Goal:** a public link that works well on real phones.

- [ ] Device matrix: at least one recent Android (Chrome) and one iPhone (Safari); one small and
      *Deferred: phone pass (laptop-first, 2026-10-23).*
      one tall screen
- [ ] Safe-area/notch check in both orientations
      *Deferred: phone pass.*
- [ ] Performance on a low-end phone; profile and fix any frame drops
      *Deferred: phone pass. Laptop Chrome holds 60 fps.*
- [x] Size: compare `-O2` vs `-Os`; strip unused assets; target < 2 MB total
      *`-O2`: 1.29 MB (406 KB gzipped). `-Os`: 1.20 MB (399 KB gzipped). Kept `-O2`; both well under 2 MB.*
- [ ] Optional `web/manifest.json` (name, icons, `display: standalone`, portrait) for
      *Skipped (optional; needs a non-C++ file, ask first). Add with the phone pass.*
      add-to-home-screen
- [x] Version string and credits on the Settings screen
- [ ] Deploy to a static host over HTTPS (GitHub Pages, Netlify or Cloudflare Pages); confirm
      *Not done: needs your hosting account. The four files in `build/web-release/` are ready to upload.*
      `.wasm` is served as `application/wasm`
- [x] README: how to play, controls, how to build

**Exit criteria**
- Public URL loads and plays all five games on the device matrix; saves persist across visits

**Phone check:** cold load on mobile data; add to home screen and launch from the icon.

---

## Phase 13 — OOP audit & documentation · S

**Goal:** make the OOP story easy to present and grade.

- [x] Audit `docs/oop-map.md` against design §13 — every row has a `file:symbol` and one-line why
- [x] Remove any concept usage that feels forced; replace it with a natural one
- [x] ~~Class diagrams~~ and ~~viva notes~~: removed 2026-10-25, duplicated `design.md` and `docs/oop-map.md`
- [x] `docs/walkthrough.md`: follow one tap from the screen to a pixel changing (touch → input →
      game → renderer → compositor)
- [x] ~~Final pass on comments~~: comments removed 2026-10-25; `docs/oop-map.md` is the only concept index

**Exit criteria**
- Someone new can find any OOP concept in the code within a minute using `oop-map.md`

---

## Stretch ideas (after Phase 13)

- A second SILVERBALL table file (no code changes — proves the data-driven design)
- Two-player hot-seat MINEFIELD race
- Replay recording: store the seed plus input per frame (`std::vector<ButtonSet>`) to a file
- Gamepad input as a fourth `IInputSource`
