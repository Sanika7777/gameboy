#include "core/App.hpp"
#include "core/Log.hpp"
#include "io/Paths.hpp"
#include "platform/SdlPlatform.hpp"

#include <memory>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace {

// Never on main()'s stack: on the web, emscripten_set_main_loop_arg(..., true) unwinds main.
std::unique_ptr<nb::App> app;

} // namespace

int main(int, char**) {
    nb::io::Paths::configure(nb::platform::SdlPlatform::assetDir(), nb::platform::SdlPlatform::saveDir());
#ifndef __EMSCRIPTEN__
    nb::Log::get().openFile(nb::io::Paths::save("neobrick.log"));
#endif
    try {
        app = std::make_unique<nb::App>();
    } catch (const nb::Error& e) {
        nb::logError("start-up failed: ", e.what(), " ", e.context());
        return 1;
    } catch (const std::exception& e) {
        nb::logError("start-up failed: ", e.what());
        return 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg([](void* a) { static_cast<nb::App*>(a)->frame(); }, app.get(), 0, true);
#else
    while (app->running()) app->frame();
    app.reset();
#endif
    return 0;
}
