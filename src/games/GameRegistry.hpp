#pragma once

#include "core/Error.hpp"
#include "games/Game.hpp"

#include <algorithm>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace nb::games {
class GameRegistry {
public:
    using Factory = std::function<std::unique_ptr<Game>(GameContext&)>;

    void add(GameInfo info, Factory make) {
        if (find(info.id)) throw ContractViolation("duplicate game id: " + info.id);
        catalog_.push_back(std::move(info));
        factories_.push_back(std::move(make));
    }

    [[nodiscard]] std::unique_ptr<Game> create(std::string_view id, GameContext& ctx) const {
        const GameInfo* g = find(id);
        if (!g) throw UnknownGameError("UNKNOWN CARTRIDGE", std::string(id));
        return factories_[static_cast<std::size_t>(g - catalog_.data())](ctx);
    }

    [[nodiscard]] const std::vector<GameInfo>& catalog() const noexcept { return catalog_; }
    [[nodiscard]] const GameInfo* find(std::string_view id) const {
        const auto it = std::ranges::find(catalog_, id, &GameInfo::id);
        return it == catalog_.end() ? nullptr : &*it;
    }

private:
    std::vector<GameInfo> catalog_;
    std::vector<Factory> factories_;
};

void registerAll(GameRegistry& registry);

}
