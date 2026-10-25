#pragma once

// Role: where read-only assets and writable saves live; set once by main() per platform.

#include <filesystem>

namespace nb::io {

class Paths {
public:
    static void configure(std::filesystem::path assets, std::filesystem::path saves) {
        assets_() = std::move(assets);
        saves_() = std::move(saves);
    }
    [[nodiscard]] static const std::filesystem::path& assetDir() { return assets_(); }
    [[nodiscard]] static const std::filesystem::path& saveDir() { return saves_(); }
    [[nodiscard]] static std::filesystem::path asset(const std::filesystem::path& rel) { return assets_() / rel; }
    [[nodiscard]] static std::filesystem::path save(const std::filesystem::path& rel) { return saves_() / rel; }

private:
    static std::filesystem::path& assets_() { static std::filesystem::path p = "assets"; return p; }
    static std::filesystem::path& saves_() { static std::filesystem::path p = "."; return p; }
};

} // namespace nb::io
