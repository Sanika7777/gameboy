# Viva notes

Two or three sentences per concept: what it is, where it is, why it's there. File paths are under
`src/`; `docs/oop-map.md` has the full table.

## Classes and construction

- **Encapsulation** — `HighScoreTable` keeps entries private and only exposes `submit`, so the
  "at most ten, always sorted" rule can't be broken from outside. `PixelBuffer` hides its raw array.
- **Constructors** — `Vec2` and `Rect` have `constexpr` default and value constructors so layout
  constants and direction tables are computed at compile time. `Rect(int w, int h)` *delegates* to
  the four-argument constructor instead of repeating it.
- **`explicit`** — `Seconds(double)` is explicit so `update(1)` can't silently mean "one second";
  `Renderer(PixelBuffer&)` is explicit so a buffer never converts into a renderer by accident.
- **`initializer_list`** — `Palette{a, b, c, d}` reads like the four colours it is; any other count
  throws `ContractViolation`.
- **Rule of Five vs Rule of Zero** — `PixelBuffer` owns a raw `Shade*`, so it needs a destructor,
  copy/move constructors and assignments; copy assignment uses copy-and-swap for the strong
  guarantee. `RgbaSurface` does the same job with `std::vector` and needs none of them.
- **RAII** — SDL objects live in `unique_ptr` with a custom deleter (`platform/SdlHandles.hpp`);
  if start-up throws after creating the window, the window is still destroyed. `Connection` unsubscribes
  a signal listener in its destructor.
- **`= delete`** — `Game`, `NonCopyable` and `Connection` delete copying: a copied game or window
  would double-free; a copied subscription would fire twice.
- **Static members** — `Game::liveInstances_` counts games alive across all types; tests check it
  returns to 0 after a game is closed. `Palette::preset` is a static lookup.
- **Friends** — `operator<<(ostream&, Vec2)` must be a free function (stream on the left) but prints
  members; `swap(PixelBuffer&, PixelBuffer&)` touches both private pointers. `GameTestAccess` is a
  friend class so tests can inspect a game's internals without making them public.
- **Method chaining** — `TextStyle` setters return `*this`:
  `TextStyle{}.font(Font::Mini).shade(Shade::Dark).align(Align::Center)`.
- **`mutable`** — `Label::width()` is `const` (measuring doesn't change the label) but caches its
  result in a `mutable` member.

## Inheritance

- **Multilevel** — `Game → GridGame → SerpentGame`: grid helpers sit in the middle layer, shared by
  SERPENT, MINEFIELD and STACKFALL.
- **Hierarchical** — five games share `Game`; three colliders share `Collider`; three voices share
  `Channel`.
- **Multiple + virtual (diamond)** — `GameObject` inherits `IUpdatable` and `IDrawable`, both of
  which inherit `Identifiable` *virtually*, so each object has one id. The most-derived class
  (e.g. `Bird`) must construct `Identifiable` itself — the rule that makes virtual bases unusual.
- **Private inheritance** — `App : private NonCopyable` and `SerpentGame : private Counted<…>`
  borrow behaviour without exposing an "is-a" relationship.
- **Protected** — `GridGame`'s constructor and helpers are protected: only games may use them.

## Polymorphism

- **Virtual functions** — `SceneStack` calls `Scene::update` on whatever screen is on top;
  `Mixer` calls `Channel::render`, which calls the virtual `sample()` of pulse/wave/noise.
- **Abstract classes** — `Game`, `Scene`, `Collider`, `IPersistence` have pure virtuals and can't be
  instantiated.
- **`final`** — every game is `final`; `PhysicsGame::updateWorld` is `final` so physics games can't
  skip substepping.
- **RTTI** — `ErrorScene` uses `dynamic_cast<const ParseError*>` to add "file / line N" only when
  the error is a parse error.
- **Overloading** — `Renderer::fill(rect, shade)` vs `fill(rect, dither, a, b)`;
  `Renderer::draw(sprite, …)` vs `draw(sheet, frame, …)`.
- **Operators** — `Vec2` arithmetic and defaulted `<=>` (which also makes `std::set<Vec2i>` work);
  `Rect::operator&` is intersection; `BinaryWriter::operator<<` serializes; `++listView` wraps.
- **Templates** — function templates `lerp/approach/clampTo`; class templates `Vec2<T>`, `Grid<T>`,
  `Signal<Args...>`, `RingBuffer<T, N>`.
- **Specialization** — `IniFile::get<bool/int/string>` are full specializations;
  `Serializer<std::vector<T>>` and `Serializer<std::array<T, N>>` are partial; `std::hash<Vec2i>`
  lets `unordered_set<Vec2i>` work.
- **Variadic / fold** — `Log::write(level, args...)` streams any values with `(os << ... << args)`;
  `overloaded{lambdas...}` builds one visitor for `std::visit`.
- **Concepts** — `template<Arithmetic T> struct Vec2` rejects `Vec2<std::string>` at the declaration.
- **CRTP** — `Counted<T>` gives each game its own static counter with no virtual calls.

## Resources, exceptions, files

- **Smart pointers** — scenes, games, game objects and input sources are `unique_ptr`; `AssetCache`
  hands out `shared_ptr` and keeps `weak_ptr` so unused assets free themselves; `Signal` keeps
  `weak_ptr` slots so the listener controls the lifetime.
- **Exception hierarchy** — `core/Error.hpp`: `AssetError`, `ParseError` (file, line),
  `SaveError`, `CorruptSaveError`, `VersionMismatchError`, `GameError`… Callers catch by category.
- **Nested exceptions** — `GameScene` catches an `AssetError` and rethrows with
  `std::throw_with_nested(GameError("CAN'T START SILVERBALL"))`; `ErrorScene` walks the chain with
  `std::rethrow_if_nested`.
- **Function-try-block** — `AssetCache`'s constructor logs which preload failed; the exception
  continues automatically.
- **Guarantees** — moves, `swap` and the audio callback are `noexcept`; `HighScoreTable::submit`
  works on a copy then swaps (strong); file writes go to `.tmp` and are renamed (strong).
- **Text I/O** — `TextReader` reads every asset line by line with `getline` and reports file:line.
- **Binary I/O** — `scores.dat` and `.sav`: little-endian fields via `Serializer<T>`, magic,
  version and CRC32, never a raw struct dump. Corrupt scores are renamed to `.bak` and replaced.

## STL and patterns

- **Containers** — `deque` for the snake body and next pieces; `set` for mine flags;
  `unordered_set` + custom hash for snake occupancy; `queue` for flood-fill BFS; `priority_queue`
  for pinball timers; `stack` for the settings breadcrumb; `bitset` for buttons; one `list` for
  sparks (trade-off documented).
- **Algorithms** — `shuffle` (7-bag, mines), `stable_sort` (scores), `rotate` (menu, pipes),
  `count_if` (adjacent mines), `all_of` (full rows), `accumulate` (bonus), `transform` (palette).
- **Patterns** — State (scene stack), Template Method (`Game::step`), Factory (registry, table
  loader), Observer (`Signal`/`EventBus`), Strategy (`ILayout`, `LcdEffect`, `ScoreOrder`),
  Composite (`Widget`/`Panel`), Flyweight (`AssetCache`), and one Singleton (`Log`) with its
  caveats written next to it.
