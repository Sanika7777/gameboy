#include "platform/SdlPlatform.hpp"

#include "core/Log.hpp"
#include "io/Binary.hpp"
#include "io/Paths.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

#include <array>
#include <fstream>

namespace nb::platform {

namespace {

constexpr const char* kDb = "neobrick";
// Every file the app may write; restored into /save at boot.
constexpr std::array<const char*, 7> kFiles{"settings.ini", "scores.dat", "serpent.sav", "minefield.sav",
                                            "stackfall.sav", "flapwing.sav", "silverball.sav"};

// Mirrors /save (in-memory) to IndexedDB with Emscripten's C API.
// Demonstrates: pure virtual / abstract classes (second IPersistence implementation)
class WebPersistence final : public io::IPersistence {
public:
    void restoreAll(std::function<void()> done) override {
        done_ = std::move(done);
        pending_ = static_cast<int>(kFiles.size());
        for (std::size_t i = 0; i < kFiles.size(); ++i) {
            requests_[i] = {this, kFiles[i]};
            emscripten_idb_async_load(kDb, kFiles[i], &requests_[i], &onLoad, &onMissing);
        }
        // Private browsing can make IndexedDB hang: never wait more than 1.5 s.
        emscripten_async_call([](void* self) { static_cast<WebPersistence*>(self)->finish(); }, this, 1500);
    }

    void commit(const std::filesystem::path& file) override {
        try {
            const auto bytes = io::readAll(file);
            emscripten_idb_async_store(kDb, file.filename().c_str(), const_cast<std::uint8_t*>(bytes.data()),
                                       static_cast<int>(bytes.size()), nullptr, nullptr,
                                       [](void*) { logWarn("IndexedDB store failed"); });
        } catch (const Error& e) {
            logWarn("commit failed: ", e.what());
        }
    }

    void remove(const std::filesystem::path& file) override {
        emscripten_idb_async_delete(kDb, file.filename().c_str(), nullptr, nullptr, nullptr);
    }

private:
    struct Request {
        WebPersistence* self;
        const char* name;
    };

    static void onLoad(void* arg, void* data, int size) {
        auto* req = static_cast<Request*>(arg);
        std::ofstream(io::Paths::save(req->name), std::ios::binary).write(static_cast<const char*>(data), size);
        req->self->oneDone();
    }
    static void onMissing(void* arg) { static_cast<Request*>(arg)->self->oneDone(); }
    void oneDone() {
        if (--pending_ == 0) finish();
    }
    void finish() {
        if (!done_) return;
        auto d = std::move(done_);
        done_ = nullptr;
        d();
    }

    std::function<void()> done_;
    int pending_ = 0;
    std::array<Request, kFiles.size()> requests_{};   // C callback contexts, one per file
};

} // namespace

std::unique_ptr<io::IPersistence> SdlPlatform::makePersistence() { return std::make_unique<WebPersistence>(); }

} // namespace nb::platform

#else

namespace nb::platform {

std::unique_ptr<io::IPersistence> SdlPlatform::makePersistence() { return std::make_unique<io::NativePersistence>(); }

} // namespace nb::platform

#endif
