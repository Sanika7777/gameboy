#pragma once

#include "games/Game.hpp"

#include <optional>

namespace nb::games {
class GridGame : public Game {
protected:
    GridGame(GameContext& ctx, Vec2i cells, int cellPx, Vec2i origin)
        : Game(ctx), cells_(cells), cellPx_(cellPx), origin_(origin) {}

    [[nodiscard]] Vec2i cellToScreen(Vec2i cell) const noexcept { return origin_ + cell * cellPx_; }
    [[nodiscard]] std::optional<Vec2i> screenToCell(Vec2i lcd) const noexcept {
        const Vec2i rel = lcd - origin_;
        if (rel.x < 0 || rel.y < 0) return std::nullopt;
        const Vec2i c{rel.x / cellPx_, rel.y / cellPx_};
        if (c.x >= cells_.x || c.y >= cells_.y) return std::nullopt;
        return c;
    }
    [[nodiscard]] Rect cellRect(Vec2i cell) const noexcept {
        const Vec2i p = cellToScreen(cell);
        return {p.x, p.y, cellPx_, cellPx_};
    }
    void drawFrame(gfx::Renderer& r) const {
        r.rect(Rect(origin_.x - 1, origin_.y - 1, cells_.x * cellPx_ + 2, cells_.y * cellPx_ + 2), gfx::Shade::Ink);
    }
    void setGeometry(Vec2i cells, int cellPx, Vec2i origin) noexcept {
        cells_ = cells;
        cellPx_ = cellPx;
        origin_ = origin;
    }

    Vec2i cells_;
    int cellPx_;
    Vec2i origin_;
};

}
