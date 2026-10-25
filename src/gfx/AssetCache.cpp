#include "gfx/AssetCache.hpp"

#include "core/Error.hpp"
#include "core/Log.hpp"

namespace nb::gfx {

// Function-try-block: a failed preload is logged with its context, then rethrown automatically.
AssetCache::AssetCache(std::filesystem::path root, const std::vector<std::string>& preloadFonts) try
    : root_(std::move(root)) {
    for (const auto& f : preloadFonts) pinned_.push_back(font(f));
} catch (const Error& e) {
    logError("asset preload failed: ", e.what(), " ", e.context());
}

template<class T, class Loader>
std::shared_ptr<const T> AssetCache::get(std::unordered_map<std::string, std::weak_ptr<const T>>& map,
                                         const std::string& rel, Loader load) {
    if (auto it = map.find(rel); it != map.end())
        if (auto alive = it->second.lock()) return alive;
    auto made = std::make_shared<const T>(load(root_ / rel));
    map[rel] = made;
    return made;
}

std::shared_ptr<const BitmapFont> AssetCache::font(const std::string& rel) {
    return get(fonts_, rel, [](const auto& p) { return BitmapFont::load(p); });
}

std::shared_ptr<const SpriteSheet> AssetCache::sprites(const std::string& rel) {
    return get(sheets_, rel, [](const auto& p) { return SpriteSheet::load(p); });
}

} // namespace nb::gfx
