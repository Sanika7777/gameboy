#pragma once

// Role: the cartridge catalogue: id → how to build that game.
// Demonstrates: Factory; std::function; std::map

#include "core/Error.hpp"
#include "games/Game.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string_view>
#include <vector>

namespace nb::games {

class GameRegistry {
public:
    using Factory = std::function<std::unique_ptr<Game>(GameContext&)>;

    void add(GameInfo info, Factory make) {
        if (factories_.contains(info.id)) throw ContractViolation("duplicate game id: " + info.id);
        factories_.emplace(info.id, std::move(make));
        catalog_.push_back(std::move(info));
    }

    [[nodiscard]] std::unique_ptr<Game> create(std::string_view id, GameContext& ctx) const {
        const auto it = factories_.find(id);
        if (it == factories_.end()) throw UnknownGameError("UNKNOWN CARTRIDGE", std::string(id));
        return it->second(ctx);
    }

    [[nodiscard]] const std::vector<GameInfo>& catalog() const noexcept { return catalog_; }
    [[nodiscard]] const GameInfo* find(std::string_view id) const {
        for (const auto& g : catalog_)
            if (g.id == id) return &g;
        return nullptr;
    }

private:
    std::map<std::string, Factory, std::less<>> factories_;
    std::vector<GameInfo> catalog_;   // insertion order = menu order
};

} // namespace nb::games
