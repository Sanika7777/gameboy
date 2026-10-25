#pragma once

// Role: loads each font/sprite file once and shares it; unused assets free themselves.
// Demonstrates: Flyweight; shared_ptr / weak_ptr; unordered_map; function-try-block

#include "gfx/BitmapFont.hpp"
#include "gfx/Sprite.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace nb::gfx {

class AssetCache {
public:
    // Preloads (and keeps alive) the given asset paths, relative to the asset directory.
    AssetCache(std::filesystem::path root, const std::vector<std::string>& preloadFonts);

    [[nodiscard]] std::shared_ptr<const BitmapFont> font(const std::string& rel);
    [[nodiscard]] std::shared_ptr<const SpriteSheet> sprites(const std::string& rel);

private:
    template<class T, class Loader>
    std::shared_ptr<const T> get(std::unordered_map<std::string, std::weak_ptr<const T>>& map,
                                 const std::string& rel, Loader load);

    std::filesystem::path root_;
    // unordered_map: lookups by exact path, order irrelevant.
    std::unordered_map<std::string, std::weak_ptr<const BitmapFont>> fonts_;
    std::unordered_map<std::string, std::weak_ptr<const SpriteSheet>> sheets_;
    std::vector<std::shared_ptr<const BitmapFont>> pinned_;
};

} // namespace nb::gfx
