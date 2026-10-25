#pragma once

// Role: keeps the save directory durable: a no-op natively, IndexedDB mirroring on the web.
// Demonstrates: pure virtual / abstract classes (interface + two implementations)

#include <filesystem>
#include <functional>

namespace nb::io {

class IPersistence {
public:
    virtual ~IPersistence() = default;
    // Bring saved files into Paths::saveDir(); `done` runs once they are available.
    virtual void restoreAll(std::function<void()> done) = 0;
    // Call after a successful write so the file survives a reload.
    virtual void commit(const std::filesystem::path& file) = 0;
    virtual void remove(const std::filesystem::path& file) = 0;
};

// Files written natively are already durable.
class NativePersistence final : public IPersistence {
public:
    void restoreAll(std::function<void()> done) override { done(); }
    void commit(const std::filesystem::path&) override {}
    void remove(const std::filesystem::path&) override {}
};

} // namespace nb::io
