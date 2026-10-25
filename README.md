# NEOBRICK

A pocket console that never existed: a 160×144 four-shade LCD, chip sound, eight buttons and five
original cartridges. Everything — engine, renderer, fonts, sound synthesis, physics, UI and all
games — is C++20, compiled to WebAssembly with Emscripten. SDL2 is the only third-party code.

| Cartridge  | What it is                    |
|------------|-------------------------------|
| SERPENT    | snake: Box, Wrap and 3 mazes, speed 1–9 |
| MINEFIELD  | mine sweeping: Rookie, Veteran, Expert |
| STACKFALL  | falling blocks with hold, ghost, DAS/ARR |
| FLAPWING   | one-tap flyer with medals and day/night |
| SILVERBALL | pinball from a table file, 3 balls, multipliers, TILT |

## Controls

| Console | Keyboard | Mouse / touch |
|---------|----------|---------------|
| D-pad   | Arrow keys or W A S D | click/press the drawn D-pad |
| A       | X or K   | A button |
| B       | Z or J   | B button |
| START   | Enter or Esc | START pill |
| SELECT  | Shift or Backspace | SELECT pill |
| Debug overlay | F1, or SELECT+START | |

Clicking or tapping on the LCD also works where it makes sense: flap in FLAPWING, dig/flag cells in
MINEFIELD (long-press flags), swipe to steer in SERPENT, hold the left/right half for the
SILVERBALL flippers.

START pauses any game (Resume / Restart / Save & Quit / Quit). Save & Quit keeps a save state; the
menu offers CONTINUE next time. SELECT on the menu opens Settings (palette, case colour, LCD effect,
reduce flashing, volume, haptics, layout, left-handed).

## Build and run

Prerequisites: [emsdk](https://github.com/emscripten-core/emsdk), CMake ≥ 3.24, Ninja.
Native builds also need the SDL2 dev package (`sudo apt-get install libsdl2-dev`).

```bash
source ~/emsdk/emsdk_env.sh

# web (the product)
cmake --preset web-release
cmake --build --preset web-release
python3 -m http.server 8080 -d build/web-release     # or: emrun --no_browser --port 8080 build/web-release/index.html
# open http://localhost:8080/index.html

# tests (native, no SDL needed)
cmake --preset native-debug
cmake --build --preset native-debug
ctest --preset native-debug --output-on-failure

# native app (needs libsdl2-dev)
./build/native-debug/neobrick
```

The web build is four static files (`index.html/.js/.wasm/.data`, about 1.3 MB, 0.4 MB gzipped)
that any static host can serve. Saves live in IndexedDB on the web and in the SDL pref path natively.

## Where to look

- `design.md` — architecture, class design, file formats, game specs
- `phases.md` — build order and what is done
- `docs/oop-map.md` — every C++ OOP concept → file:symbol → why it's there
- `docs/walkthrough.md` — one tap, from the screen to a pixel

All games, art, fonts and sounds are original.
