# OOP concept map

Where each C++ OOP feature lives in NEOBRICK, and why it belongs there. Live version of design §13.
Paths are relative to `src/` unless they start with `tests/`. Source files carry no comments; this table is the
only concept index.

## Fundamentals

| Concept | Where (`file:symbol`) | Why here |
|---------|-----------------------|----------|
| Classes, objects, encapsulation | `gfx/PixelBuffer.hpp:PixelBuffer`, `io/HighScoreTable.hpp:HighScoreTable` | PixelBuffer hides its raw array behind `operator()`/`at`; HighScoreTable never lets callers break its "≤ 10 entries, always sorted" invariant |
| Abstraction | `input/InputSources.hpp:IInputSource`, `scenes/Scene.hpp:Scene`, `games/Game.hpp:Game` | Callers work with "an input source", "a screen", "a game" and never know the concrete type |
| Default / parameterized constructors | `core/Vec2.hpp:Vec2`, `core/Rect.hpp:Rect`, `gfx/PixelBuffer.hpp:PixelBuffer` | Value types need a sensible zero state and a direct way to build one |
| `constexpr` constructors | `core/Vec2.hpp:Vec2`, `core/Rect.hpp:Rect` | Directions, kick tables and layout constants are compile-time values (`static_assert(a + b == …)` in `tests/test_core.cpp`) |
| Delegating constructor | `core/Rect.hpp:Rect(int w, int h)` | A size-only rect is "a rect at the origin", so it forwards to the full constructor |
| `explicit` constructor | `gfx/Renderer.hpp:Renderer` | Stops a PixelBuffer turning into a Renderer by accident |
| `initializer_list` constructor | `gfx/Palette.hpp:Palette` | A palette is literally "these four colours"; the constructor throws `ContractViolation` for any other count |
| Copy / move constructors & assignment | `gfx/PixelBuffer.hpp:PixelBuffer` | Deep copy for sprite duplication, pointer-stealing move for cheap returns, copy-and-swap for the strong guarantee |
| Rule of Five vs Rule of Zero | `gfx/PixelBuffer.hpp:PixelBuffer` vs `gfx/RgbaSurface.hpp:RgbaSurface` | Same job, two ownership styles: raw `Shade*` needs all five members; `std::vector` needs none |
| Destructors, RAII | `platform/SdlHandles.hpp:SdlWindow/SdlRenderer/SdlTexture`, `platform/SdlPlatform.hpp:SdlPlatform::Init`, `core/Signal.hpp:Connection` | `unique_ptr` + custom deleter frees each SDL object exactly once even when start-up throws halfway; `Init` is declared first so `SDL_Quit` runs last; dropping a `Connection` unsubscribes |
| `= delete` / `= default` | `core/App.hpp:App`, `games/Game.hpp:Game`, `core/Signal.hpp:Connection` | Copying a window, a running game or a subscription would double-free or double-fire; `= default` keeps the moves |
| Static members / functions | `games/Game.hpp:Game::liveInstances_`, `gfx/Palette.hpp:Palette::preset`, `io/Paths.hpp:Paths` | One counter shared by all games (leak check: back to 0 after leaving a game); presets and paths belong to the class, not to an instance |
| Friend function | `core/Vec2.hpp:operator<<`, `gfx/PixelBuffer.hpp:swap` | `<<` needs the stream on the left; `swap` touches both objects' private pointers |
| Friend class | `tests/GameTestAccess.hpp:GameTestAccess` (befriended in `games/Game.hpp` and every game) | Tests read and poke internals (snake body, mine grid) without making them public |
| `this` and method chaining | `gfx/BitmapFont.hpp:TextStyle` | `TextStyle{}.font(Font::Mini).shade(Shade::Dark).align(Align::Center)` returns `*this` from each setter |
| Namespaces (nested) | `nb::gfx`, `nb::io`, `nb::games::serpent`, … | Each layer and each cartridge has its own scope; `nb::games::serpent::Direction` can't collide with anything |
| `enum class` | `input/Button.hpp:Button`, `gfx/Shade.hpp:Shade`, `games/serpent/SerpentGame.hpp:Direction` | Scoped, no implicit int conversion: a `Shade` can't be passed where a `Button` is expected |

## Inheritance

| Kind | Where (`file:symbol`) | Why here |
|------|-----------------------|----------|
| Single | `ui/Widget.hpp:ListView : Widget`, `input/InputSources.hpp:KeyboardSource : IInputSource` | One "is-a" relationship each |
| Multilevel | `games/GridGame.hpp:GridGame`, `games/serpent/SerpentGame.hpp:SerpentGame` | `Game → GridGame → SerpentGame`: grid helpers (`cellToScreen`, `screenToCell`, `drawFrame`) sit between the general game and the three grid games |
| Hierarchical | `games/Game.hpp:Game` → five games; `games/silverball/Collider.hpp:Collider` → three colliders; `audio/Channel.hpp:Channel` → three voices | Many siblings share one base |
| Multiple (interfaces) | `games/GameObject.hpp:GameObject : IUpdatable, IDrawable` | A world object is both something that updates and something that draws |
| Hybrid / diamond, `virtual` inheritance | `games/GameObject.hpp:Identifiable`, `IUpdatable`, `IDrawable` | Both interfaces derive `virtual`ly from `Identifiable`, so a GameObject has exactly one `Identifiable` subobject; each concrete object (e.g. `flapwing::Bird`) constructs the virtual base itself |
| Protected members / constructors | `games/GridGame.hpp:GridGame`, `games/PhysicsGame.hpp:PhysicsGame` | Only subclasses may construct these or use their helpers |

## Polymorphism

| Kind | Where (`file:symbol`) | Why here |
|------|-----------------------|----------|
| Virtual functions, `override` | `scenes/Scene.hpp:Scene::update`, `games/Game.hpp:Game::updateWorld`, `audio/Channel.hpp:Channel::sample` | The stack, the game host and the mixer call one interface; each screen/game/voice supplies its own body |
| Pure virtual / abstract classes | `games/Game.hpp:Game`, `scenes/Scene.hpp:Scene`, `games/silverball/Collider.hpp:Collider`, `io/IPersistence.hpp:IPersistence` | Can't instantiate "a game" — only SERPENT, MINEFIELD, … |
| `final` classes and methods | every game class; `games/PhysicsGame.hpp:PhysicsGame::updateWorld` | Leaves are sealed; `updateWorld` is final so no physics game can skip substepping |
| Virtual destructors | every polymorphic base (`Scene`, `Game`, `Collider`, `Channel`, `IInputSource`, `ILayout`, `LcdEffect`, `IPersistence`, `Widget`, `Identifiable`) | Objects are owned through `unique_ptr<Base>`; `-Wnon-virtual-dtor` enforces it |
| RTTI, `dynamic_cast` | `scenes/ErrorScene.cpp:ErrorScene::describe`, `scenes/GameScene.cpp:GameScene::onHidden` | Only a `ParseError` has a file and line; the auto-pause checks whether the top scene is already a `PauseScene` |
| Function overloading | `gfx/Renderer.hpp:Renderer::fill`, `Renderer::draw` | Solid vs dithered fill; single sprite vs sheet frame |
| Operator overloading | `core/Vec2.hpp:Vec2` (`+ - * <=> <<`), `core/Rect.hpp:Rect::operator&`, `core/Grid.hpp:Grid::operator()`, `io/Binary.hpp:BinaryWriter::operator<<`, `ui/Widget.hpp:ListView::operator++` | Maths reads like maths; `rect & clip` is intersection; `writer << score` serializes; `++list` moves the cursor with wrap-around |
| Function templates | `scenes/Common.hpp:fmt` | One fixed-buffer formatter for any printf argument list |
| Class templates | `core/Vec2.hpp:Vec2<T>`, `core/Grid.hpp:Grid<T>`, `core/Signal.hpp:Signal<Args...>`, `core/RingBuffer.hpp:RingBuffer<T,N>` | Same structure, different element types |
| Full / partial specialization | `io/IniFile.hpp:IniFile::get<bool/int/std::string>`; `io/Binary.hpp:Serializer<std::string>` (full), `Serializer<std::vector<T>>` and `Serializer<std::array<T,N>>` (partial); `core/Vec2.hpp:std::hash<nb::Vec2i>` | Parsing "true" differs from parsing "42"; strings and containers need a length prefix; `unordered_set<Vec2i>` needs a hash |
| Variadic templates, fold expressions | `core/Log.hpp:Log::write` (`(os << ... << args)`), `core/Signal.hpp:Signal<Args...>`, `core/PlatformEvent.hpp:overloaded` | Log any mix of values; signals with any argument list; one visitor from many lambdas |
| Concepts (C++20) | `core/Vec2.hpp:Arithmetic` | `Vec2<std::string>` fails at the declaration with a clear message |

## Resource management, exceptions, files

| Concept | Where (`file:symbol`) | Why here |
|---------|-----------------------|----------|
| `unique_ptr` ownership | `scenes/Scene.hpp:SceneStack::stack_`, `scenes/GameScene.hpp:GameScene::game_`, `games/PhysicsGame.hpp:PhysicsGame::objects_`, `input/InputManager.hpp:InputManager::sources_` | Exactly one owner each; destruction order is automatic |
| `shared_ptr` / `weak_ptr` | `core/Signal.hpp:Signal` | A signal holds only `weak_ptr`s, so listeners own their subscription |
| Move semantics, perfect forwarding | `gfx/PixelBuffer.hpp:PixelBuffer(PixelBuffer&&)`, `scenes/Scene.hpp:SceneStack::push<T>(Args&&...)` | Sprites are moved into sheets, not copied; scene constructor arguments arrive untouched |
| Custom exception hierarchy | `core/Error.hpp` | Callers catch by category (`AssetError`, `SaveError`, …) and `ErrorScene` can explain each kind |
| `try` / `catch` / `throw`, rethrow | `core/App.cpp:App::frame`, `io/HighScoreTable.cpp:ScoreBook::load`, `scenes/GameScene.cpp:GameScene::GameScene` | The frame boundary turns any escape into an error screen; save loading recovers |
| Nested exceptions | `scenes/GameScene.cpp:GameScene::GameScene` (`std::throw_with_nested`), `scenes/ErrorScene.cpp:ErrorScene::describe` (`std::rethrow_if_nested`) | "CAN'T START SILVERBALL" wraps the parse error underneath, and the screen shows both |
| `noexcept`, exception-safety guarantees | `gfx/PixelBuffer.hpp` (moves, `swap`), `audio/Mixer.hpp:Mixer::render`, `core/App.hpp:App::frame`; strong: `io/HighScoreTable.cpp:HighScoreTable::submit`, `io/Settings.cpp:Settings::apply`, `io/Binary.cpp:atomicWrite` | The audio callback must never throw; a failed score submit or settings write leaves the old state untouched |
| Text file I/O (`ifstream`, `getline`) | `io/TextReader.cpp:TextReader`, used by fonts, sprites, palettes, `settings.ini`, mazes, `stackfall.txt`, `sfx.txt`, tables | Every asset is a readable text file; every parser reports file and line |
| Binary file I/O (`ios::binary`, byte order, CRC) | `io/Binary.hpp:BinaryWriter/BinaryReader/Serializer`, `io/HighScoreTable.cpp:ScoreBook::encode/decode`, `scenes/SaveState.cpp` | `scores.dat` and `<game>.sav`: little-endian field by field, magic, version, CRC32 — never a raw struct dump |
| `std::filesystem` (rename, exists) | `io/Binary.cpp:atomicWrite`, `io/HighScoreTable.cpp:ScoreBook::load` (`.bak`), `scenes/SaveState.cpp:hasSave` | Write `.tmp` then rename: a crash never leaves half a file |
| Append-mode logging | `core/Log.hpp:Log::openFile` | `neobrick.log` keeps history across native runs |

## STL

| Item | Where (`file:symbol`) | Why here |
|------|-----------------------|----------|
| `array` | `games/stackfall/StackfallGame.hpp:kShapes/kKicks/bag_`, `core/RingBuffer.hpp:RingBuffer::buf_`, `audio/Mixer.hpp:Mixer::channels_` | Fixed sizes known at compile time; rotation tables are computed `constexpr` |
| `vector` | everywhere (scene stack, objects, sources, catalog) | The default container |
| `deque` | `games/serpent/SerpentGame.hpp:body_`, `games/stackfall/StackfallGame.hpp:next_`, `games/flapwing/FlapwingGame.hpp:pipes_`, `games/silverball/SilverballGame.hpp:nudges_` | Cheap push/pop at both ends: snake head in, tail out; pieces queued at the back, taken from the front |
| `list` | `games/silverball/SilverballGame.hpp:sparks_` | Sparks die at arbitrary positions; `list::remove_if` erases in O(1) without moving survivors. **Trade-off:** a `vector` + `std::erase_if` is contiguous and usually faster for this little data; the list is kept as the single, documented example |
| `map` | `io/IniFile.hpp:IniFile::sections_`, `games/silverball/TableLoader.cpp:factories()`, `audio/Sfx.hpp:SfxLibrary::defs_` | Ordered and searchable by `string_view` (`std::less<>`), so lookups don't allocate |
| `set` | `games/minefield/MinefieldGame.hpp:flags_` | Ordered by `Vec2`'s defaulted `<=>`; deterministic iteration for save files |
| `unordered_set` + custom hash | `games/serpent/SerpentGame.hpp:occupied_` with `std::hash<Vec2i>` | O(1) "is this cell part of the snake?". Note: inserting allocates a node each move (a `Grid<uint8_t>` would avoid that) — kept because it's the design's showcase |
| `queue` | `games/minefield/MinefieldGame.cpp:flood` (BFS), `games/serpent/SerpentGame.hpp:turns_` | First in, first out: cells to open; buffered turns |
| `priority_queue` | `games/silverball/SilverballGame.hpp:timers_` | Min-heap (`std::greater<>`): the next timer to fire is always on top |
| `stack` | `scenes/SettingsScene.hpp:pages_` | Sub-menu breadcrumb: only "current" and "back" matter |
| `bitset` | `input/Button.hpp:ButtonSet` | Eight buttons in one byte; edges are `held & ~previous` |
| `optional`, `variant`, `visit` | `input/InputState.hpp:lcdTap/gesture`, `games/silverball/Collider.hpp:Collider::test`; `core/PlatformEvent.hpp:PlatformEvent` + `core/App.cpp:App::handle` | "Maybe a tap", "maybe a contact"; one event type that is exactly one of six kinds |
| `string_view`, `span` | `io/TextReader.hpp:tokens`, `gfx/PixelBuffer.hpp:row`, `audio/Mixer.hpp:render` | Parse without copying; hand out buffer rows and audio blocks without ownership |
| `function`, lambdas | `games/GameRegistry.hpp:Factory`, `core/Signal.hpp:Slot`, `games/silverball/TableLoader.cpp:factories()` | Store "how to build X" and "what to do when Y" as values |
| `pair`/`tuple`, structured bindings | `io/IniFile.cpp:text` (`for (const auto& [name, keys] : sections_)`), `io/TextReader.cpp:parseInt` (`auto [p, ec] = from_chars`) | Map iteration and multi-value results read clearly |
| Algorithms | `shuffle` (7-bag, mines), `stable_sort` (scores), `rotate` (menu carousel, pipe recycling), `remove_if`/`erase_if` (sparks, signals, mine candidates), `count_if` (adjacent mines), `all_of` (full rows), `accumulate` (end-of-ball bonus), `transform` (shade → colour in `gfx/Compositor.cpp`), `iota` (mine candidates), `clamp` | Each replaces a hand-written loop with a named intent |
| Iterators | `core/Grid.hpp:Grid::begin/end`, `Grid::Neighbours::begin/end` | Range-for and STL algorithms work directly on grids and neighbour lists |

## Design patterns

| Pattern | Where (`file:symbol`) | Why here |
|---------|-----------------------|----------|
| State | `scenes/Scene.hpp:SceneStack` and the scenes | Each screen is a state object; transitions are push/pop/replace, applied after the update |
| Template Method | `games/Game.hpp:Game::step/render` | The order (input → world; world → HUD) is fixed once; games fill in the steps |
| Factory | `games/GameRegistry.hpp:GameRegistry`, `games/silverball/TableLoader.cpp:factories()` | Build a game from its id; build a table element from its keyword |
| Observer | `core/Signal.hpp:Signal/EventBus` | Games say "play eat"; audio and haptics listen, and neither knows the other |
| Strategy | `device/ILayout.hpp:ILayout`, `gfx/Compositor.hpp:LcdEffect`, `io/HighScoreTable.hpp:ScoreOrder` | Swap the console layout, the LCD look and the score ordering without touching callers |
| Singleton (with caveats) | `core/Log.hpp:Log::get` | The only singleton; the header explains why the rest of the code passes objects explicitly |
