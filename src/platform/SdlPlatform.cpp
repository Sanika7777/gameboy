#include "platform/SdlPlatform.hpp"

#include "core/Error.hpp"
#include "core/Log.hpp"

#include <SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#include <algorithm>
#include <string>

namespace nb::platform {
void SdlDeleter::operator()(SDL_Window* w) const noexcept { SDL_DestroyWindow(w); }
void SdlDeleter::operator()(SDL_Renderer* r) const noexcept { SDL_DestroyRenderer(r); }
void SdlDeleter::operator()(SDL_Texture* t) const noexcept { SDL_DestroyTexture(t); }

namespace {
[[noreturn]] void fail(const char* what) { throw PlatformError("PLATFORM ERROR", std::string(what) + ": " + SDL_GetError()); }

Key toKey(SDL_Keycode k) noexcept {
    switch (k) {
    case SDLK_UP: return Key::Up;
    case SDLK_DOWN: return Key::Down;
    case SDLK_LEFT: return Key::Left;
    case SDLK_RIGHT: return Key::Right;
    case SDLK_w: return Key::W;
    case SDLK_a: return Key::A;
    case SDLK_s: return Key::S;
    case SDLK_d: return Key::D;
    case SDLK_x: return Key::X;
    case SDLK_k: return Key::K;
    case SDLK_z: return Key::Z;
    case SDLK_j: return Key::J;
    case SDLK_RETURN: case SDLK_KP_ENTER: return Key::Enter;
    case SDLK_ESCAPE: return Key::Escape;
    case SDLK_RSHIFT: case SDLK_LSHIFT: return Key::RShift;
    case SDLK_BACKSPACE: return Key::Backspace;
    case SDLK_F1: return Key::F1;
    default: return Key::Unknown;
    }
}

#ifdef __EMSCRIPTEN__
bool onVisibility(int, const EmscriptenVisibilityChangeEvent* e, void* user) {
    static_cast<SdlPlatform*>(user)->pushEvent(VisibilityEvent{!e->hidden});
    return false;
}
#endif

}

SdlPlatform::Init::Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) fail("SDL_Init");
}
SdlPlatform::Init::~Init() { SDL_Quit(); }

SdlPlatform::SdlPlatform() {
    window_.reset(SDL_CreateWindow("NEOBRICK", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 540,
                                   SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI));
    if (!window_) fail("SDL_CreateWindow");
    renderer_.reset(SDL_CreateRenderer(window_.get(), -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC));
    if (!renderer_) fail("SDL_CreateRenderer");
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
    events_.reserve(64);
#ifdef __EMSCRIPTEN__
    emscripten_set_visibilitychange_callback(this, false, &onVisibility);
#endif
}

SdlPlatform::~SdlPlatform() {
    if (audioDevice_) SDL_CloseAudioDevice(audioDevice_);
}

Vec2i SdlPlatform::physSize() const {
    int w = 0, h = 0;
    SDL_GetRendererOutputSize(renderer_.get(), &w, &h);
    return {w, h};
}

std::span<const PlatformEvent> SdlPlatform::pumpEvents() {
    events_.erase(events_.begin(), events_.begin() + static_cast<std::ptrdiff_t>(std::min(seen_, events_.size())));

    int winW = 1, winH = 1;
    SDL_GetWindowSize(window_.get(), &winW, &winH);
    const Vec2i phys = physSize();
    auto toPhys = [&](int x, int y) { return Vec2i{x * phys.x / std::max(1, winW), y * phys.y / std::max(1, winH)}; };

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT: events_.push_back(QuitEvent{}); break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            if (const Key k = toKey(e.key.keysym.sym); k != Key::Unknown)
                events_.push_back(KeyEvent{k, e.type == SDL_KEYDOWN, e.key.repeat != 0});
            break;
        case SDL_FINGERDOWN:
        case SDL_FINGERMOTION:
        case SDL_FINGERUP: {
            const auto phase = e.type == SDL_FINGERDOWN ? PointerPhase::Down
                             : e.type == SDL_FINGERUP   ? PointerPhase::Up : PointerPhase::Move;
            events_.push_back(FingerEvent{static_cast<std::int64_t>(e.tfinger.fingerId), phase,
                                          {static_cast<int>(e.tfinger.x * phys.x), static_cast<int>(e.tfinger.y * phys.y)}});
            break;
        }
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
            if (e.button.which != SDL_TOUCH_MOUSEID && e.button.button == SDL_BUTTON_LEFT)
                events_.push_back(MouseEvent{e.type == SDL_MOUSEBUTTONDOWN ? PointerPhase::Down : PointerPhase::Up,
                                             toPhys(e.button.x, e.button.y)});
            break;
        case SDL_MOUSEMOTION:
            if (e.motion.which != SDL_TOUCH_MOUSEID && (e.motion.state & SDL_BUTTON_LMASK))
                events_.push_back(MouseEvent{PointerPhase::Move, toPhys(e.motion.x, e.motion.y)});
            break;
        case SDL_WINDOWEVENT:
            switch (e.window.event) {
            case SDL_WINDOWEVENT_SIZE_CHANGED: events_.push_back(ResizeEvent{phys}); break;
            case SDL_WINDOWEVENT_MINIMIZED: events_.push_back(VisibilityEvent{false}); break;
            case SDL_WINDOWEVENT_RESTORED: events_.push_back(VisibilityEvent{true}); break;
            default: break;
            }
            break;
        default: break;
        }
    }
    seen_ = events_.size();
    return events_;
}

void SdlPlatform::present(std::span<const std::uint32_t> pixels, Vec2i size, Vec2i offset, int scale, std::optional<Rect> grid) {
    if (!texture_ || textureSize_ != size) {
        texture_.reset(SDL_CreateTexture(renderer_.get(), SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, size.x, size.y));
        if (!texture_) fail("SDL_CreateTexture");
        textureSize_ = size;
    }
    SDL_UpdateTexture(texture_.get(), nullptr, pixels.data(), size.x * static_cast<int>(sizeof(std::uint32_t)));

    const std::uint32_t c = pixels[pixels.size() - static_cast<std::size_t>(size.x)];
    SDL_SetRenderDrawColor(renderer_.get(), (c >> 16) & 0xFF, (c >> 8) & 0xFF, c & 0xFF, 0xFF);
    SDL_RenderClear(renderer_.get());
    const SDL_Rect dst{offset.x, offset.y, size.x * scale, size.y * scale};
    SDL_RenderCopy(renderer_.get(), texture_.get(), nullptr, &dst);

    if (grid && scale >= 4) {
        SDL_SetRenderDrawBlendMode(renderer_.get(), SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_.get(), 0, 0, 0, 40);
        const int x0 = offset.x + grid->x * scale, y0 = offset.y + grid->y * scale;
        for (int i = 1; i < grid->w; ++i) {
            const SDL_Rect line{x0 + i * scale, y0, 1, grid->h * scale};
            SDL_RenderFillRect(renderer_.get(), &line);
        }
        for (int i = 1; i < grid->h; ++i) {
            const SDL_Rect line{x0, y0 + i * scale, grid->w * scale, 1};
            SDL_RenderFillRect(renderer_.get(), &line);
        }
    }
    SDL_RenderPresent(renderer_.get());
}

void SdlPlatform::openAudio(AudioFn fn, void* user) {
    audioFn_ = fn;
    audioUser_ = user;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        logWarn("no audio: ", SDL_GetError());
        return;
    }
    SDL_AudioSpec want{};
    want.freq = 22050;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = 512;
    want.userdata = this;
    want.callback = [](void* self, Uint8* stream, int len) {
        auto* p = static_cast<SdlPlatform*>(self);
        p->audioFn_(p->audioUser_, reinterpret_cast<std::int16_t*>(stream), len / 2);
    };
    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &want, nullptr, 0);
    if (!audioDevice_) {
        logWarn("no audio device: ", SDL_GetError());
        return;
    }
    SDL_PauseAudioDevice(audioDevice_, 0);
}

void SdlPlatform::pauseAudio(bool paused) {
    if (audioDevice_) SDL_PauseAudioDevice(audioDevice_, paused ? 1 : 0);
}

void SdlPlatform::vibrate(int ms) {
#ifdef __EMSCRIPTEN__
    emscripten_vibrate(ms);
#else
    (void)ms;
#endif
}

std::filesystem::path SdlPlatform::assetDir() {
#ifdef __EMSCRIPTEN__
    return "/assets";
#else
    char* base = SDL_GetBasePath();
    std::filesystem::path p = base ? std::filesystem::path(base) / "assets" : std::filesystem::path("assets");
    SDL_free(base);
    return p;
#endif
}

std::filesystem::path SdlPlatform::saveDir() {
#ifdef __EMSCRIPTEN__
    std::filesystem::path p = "/save";
#else
    char* pref = SDL_GetPrefPath("neobrick", "neobrick");
    std::filesystem::path p = pref ? std::filesystem::path(pref) : std::filesystem::path("save");
    SDL_free(pref);
#endif
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
    return p;
}

}
