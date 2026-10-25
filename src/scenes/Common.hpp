#pragma once

// Role: small drawing helpers shared by the menu-style scenes.

#include "gfx/Renderer.hpp"

#include <array>
#include <cstdio>
#include <string_view>

namespace nb::scenes {

inline void titleBar(gfx::Renderer& r, std::string_view title) {
    r.fill(Rect(0, 0, 160, 11), gfx::Shade::Ink);
    r.text(title, {80, 2}, gfx::TextStyle{}.shade(gfx::Shade::Paper).align(gfx::Align::Center));
}

inline void footer(gfx::Renderer& r, std::string_view hint) {
    r.fill(Rect(0, 136, 160, 8), gfx::Shade::Light);
    r.text(hint, {80, 137}, gfx::TextStyle{}.font(gfx::Font::Mini).align(gfx::Align::Center));
}

// Fixed buffer formatting: no heap allocation in draw paths.
template<class... Args>
std::array<char, 32> fmt(const char* f, Args... args) {
    std::array<char, 32> buf{};
    std::snprintf(buf.data(), buf.size(), f, args...);
    return buf;
}

} // namespace nb::scenes
