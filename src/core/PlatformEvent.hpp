#pragma once

#include "core/Vec2.hpp"

#include <cstdint>
#include <variant>

namespace nb {
enum class Key : std::uint8_t {
    Unknown, Up, Down, Left, Right, W, A, S, D, X, K, Z, J, Enter, Escape, RShift, Backspace, F1
};

enum class PointerPhase : std::uint8_t { Down, Move, Up };

struct KeyEvent { Key key; bool down; bool repeat; };
struct FingerEvent { std::int64_t id; PointerPhase phase; Vec2i phys; };
struct MouseEvent { PointerPhase phase; Vec2i phys; };
struct ResizeEvent { Vec2i phys; };
struct VisibilityEvent { bool visible; };
struct QuitEvent {};

using PlatformEvent = std::variant<KeyEvent, FingerEvent, MouseEvent, ResizeEvent, VisibilityEvent, QuitEvent>;

template<class... Fs>
struct overloaded : Fs... {
    using Fs::operator()...;
};

}
