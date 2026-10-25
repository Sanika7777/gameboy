#pragma once

#include <filesystem>
#include <functional>

namespace nb::io {
class IPersistence {
public:
    virtual ~IPersistence() = default;
    virtual void restoreAll(std::function<void()> done) = 0;
    virtual void commit(const std::filesystem::path& file) = 0;
    virtual void remove(const std::filesystem::path& file) = 0;
};

class NativePersistence final : public IPersistence {
public:
    void restoreAll(std::function<void()> done) override { done(); }
    void commit(const std::filesystem::path&) override {}
    void remove(const std::filesystem::path&) override {}
};

}
