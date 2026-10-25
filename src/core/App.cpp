#include "core/App.hpp"

#include "core/Log.hpp"
#include "gfx/Palette.hpp"
#include "io/Paths.hpp"
#include "scenes/BootScene.hpp"
#include "scenes/Common.hpp"
#include "scenes/ErrorScene.hpp"

#include <ctime>

namespace nb {
using gfx::Shade;
using input::Button;

App::App()
    : persistence_(platform::SdlPlatform::makePersistence()),
      mainFont_(gfx::BitmapFont::load(io::Paths::asset("fonts/pixel5x7.fnt"))),
      miniFont_(gfx::BitmapFont::load(io::Paths::asset("fonts/mini3x5.fnt"))),
      lcd_(160, 144),
      renderer_(lcd_),
      shell_(miniFont_),
      input_(shell_),
      sfx_(audio::SfxLibrary::load(io::Paths::asset("audio/sfx.txt"))),
      settings_(io::Paths::save("settings.ini"), *persistence_),
      scores_(io::Paths::save("scores.dat"), *persistence_),
      rng_(static_cast<std::uint32_t>(std::time(nullptr))),
      services_{scenes_, events_, rng_, settings_, scores_, registry_, *persistence_} {
    try {
        throw std::runtime_error("probe");
    } catch (const std::exception&) {
        logInfo("exceptions OK");
    }
    renderer_.setFonts(&mainFont_, &miniFont_);
    gfx::Palette::loadPresets(io::Paths::asset("data/palettes.txt"));
    games::registerAll(registry_);

    connections_.push_back(events_.playSfx.connect([this](std::string_view name) {
        if (const auto* d = sfx_.find(name)) mixer_.play(*d);
    }));
    connections_.push_back(events_.vibrate.connect([this](int ms) {
        if (settings_.get().haptics) platform_.vibrate(ms);
    }));
    connections_.push_back(events_.toast.connect([this](std::string_view msg) { toast_.show(std::string(msg)); }));

    relayout(platform_.physSize());
    platform_.openAudio(&App::renderAudio, this);
    persistence_->restoreAll([this] { start(); });
}

App::~App() { platform_.pauseAudio(true); }

void App::renderAudio(void* self, std::int16_t* out, int samples) noexcept {
    static_cast<App*>(self)->mixer_.render({out, static_cast<std::size_t>(samples)});
}

void App::start() {
    settings_.load();
    applySettings();
    if (const auto msg = scores_.load()) toast_.show(*msg);
    scenes_.push<scenes::BootScene>(services_);
    scenes_.applyPending();
    ready_ = true;
    logInfo("NEOBRICK ready");
}

void App::relayout(Vec2i phys) {
    phys_ = phys;
    shell_.configure(device::makeLayout(settings_.get().layout, phys), phys);
}

void App::applySettings() {
    const auto& v = settings_.get();
    const bool layoutChanged = firstLayout_ || v.layout != applied_.layout;
    shell_.setTheme(v.caseTheme);
    shell_.setLeftHanded(v.leftHanded);
    shell_.setLed(v.volume > 0);
    compositor_.setEffect(gfx::makeLcdEffect(v.lcdEffect));
    mixer_.setVolume(v.volume);
    applied_ = v;
    firstLayout_ = false;
    if (layoutChanged) relayout(phys_);
}

void App::handle(const PlatformEvent& e) {
    std::visit(overloaded{
                   [&](const QuitEvent&) { running_ = false; },
                   [&](const ResizeEvent& r) { relayout(r.phys); },
                   [&](const VisibilityEvent& v) {
                       if (visible_ == v.visible) return;
                       visible_ = v.visible;
                       platform_.pauseAudio(!visible_);
                       if (!visible_ && ready_) {
                           scenes_.hidden();
                           scenes_.applyPending();
                       }
                       clock_.reset();
                   },
                   [&](const KeyEvent& k) {
                       if (k.down && k.key == Key::F1) debug_ = !debug_;
                       input_.handle(e);
                   },
                   [&](const auto&) { input_.handle(e); },
               },
               e);
}

void App::frame() noexcept {
    try {
        for (const auto& e : platform_.pumpEvents()) handle(e);
        if (const Vec2i p = platform_.physSize(); p != phys_) relayout(p);

        if (!ready_) {
            renderer_.clear(Shade::Paper);
            renderer_.text("LOADING", {80, 68}, gfx::TextStyle{}.align(gfx::Align::Center));
        } else {
            if (!(settings_.get() == applied_)) applySettings();
            const int steps = visible_ ? clock_.advance() : 0;
            const auto& in = input_.state();
            if (steps > 0) {
                input_.beginFrame();
                if (in.anyPressed()) events_.vibrate.emit(10);
                if (in.held(Button::Select) && in.held(Button::Start) &&
                    (in.pressed(Button::Select) || in.pressed(Button::Start)))
                    debug_ = !debug_;
            }
            for (int i = 0; i < steps; ++i) {
                scenes_.update(in, kStep);
                input_.endStep();
                toast_.update();
                if (fadeFrames_ > 0) --fadeFrames_;
            }
            if (scenes_.takeChanged()) fadeFrames_ = 12;

            renderer_.setCamera({0, 0});
            renderer_.resetClip();
            scenes_.draw(renderer_);
            renderer_.setCamera({0, 0});
            renderer_.resetClip();
            toast_.draw(renderer_);
            if (debug_) drawDebug();
            shell_.setPressed(in.heldSet());
        }

        compositor_.setFade((fadeFrames_ + 3) / 4);
        auto& face = shell_.beginFrame();
        compositor_.compose(lcd_, gfx::Palette::preset(settings_.get().palette), face, shell_.lcdOrigin());
        const auto& vp = shell_.viewport();
        const auto lcd = shell_.lcdOrigin();
        platform_.present(face.pixels(), face.size(), vp.offset, vp.scale,
                          compositor_.effect().wantsPixelGrid() ? std::optional<Rect>(Rect(lcd.x, lcd.y, 160, 144)) : std::nullopt);
    } catch (...) {
        showError(std::current_exception());
    }
}

void App::showError(std::exception_ptr e) noexcept {
    try {
        if (!ready_) {
            try {
                std::rethrow_exception(e);
            } catch (const std::exception& ex) {
                logError("fatal before start: ", ex.what());
            }
            running_ = false;
            return;
        }
        scenes_.resetNow(std::make_unique<scenes::ErrorScene>(services_, e));
    } catch (...) {
        logError("could not show the error screen");
        running_ = false;
    }
}

void App::drawDebug() {
    const auto& in = input_.state();
    static constexpr const char* kNames[] = {"U", "D", "L", "R", "A", "B", "ST", "SE"};
    renderer_.fill(Rect(0, 0, 160, 8), Shade::Ink);
    int x = 2;
    for (int b = 0; b < input::kButtonCount; ++b) {
        const bool on = in.held(static_cast<Button>(b));
        x += 2 + renderer_.text(kNames[b], {x, 1}, gfx::TextStyle{}.font(gfx::Font::Mini).shade(on ? Shade::Paper : Shade::Dark));
    }
    if (const auto t = in.lcdTouch()) {
        renderer_.text(scenes::fmt("%d,%d", t->x, t->y).data(), {110, 1}, gfx::TextStyle{}.font(gfx::Font::Mini).shade(Shade::Paper));
        renderer_.circle(*t, 3, Shade::Ink);
    }
}

}
