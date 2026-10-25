#pragma once

// Role: owning smart-pointer aliases for SDL objects.
// Demonstrates: destructors, RAII (unique_ptr + custom deleter)

#include <memory>

// Forward declarations match SDL's own `typedef struct SDL_X SDL_X;`, so this header stays SDL-free.
struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace nb::platform {

struct SdlDeleter {
    void operator()(SDL_Window* w) const noexcept;
    void operator()(SDL_Renderer* r) const noexcept;
    void operator()(SDL_Texture* t) const noexcept;
};

using SdlWindow   = std::unique_ptr<SDL_Window, SdlDeleter>;
using SdlRenderer = std::unique_ptr<SDL_Renderer, SdlDeleter>;
using SdlTexture  = std::unique_ptr<SDL_Texture, SdlDeleter>;

} // namespace nb::platform
