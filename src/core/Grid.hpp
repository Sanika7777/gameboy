#pragma once

#include "core/Rect.hpp"

#include <algorithm>
#include <array>
#include <type_traits>
#include <vector>

namespace nb {
template<class T>
class Grid {
    static_assert(!std::is_same_v<T, bool>, "use Grid<std::uint8_t> instead of Grid<bool>");

public:
    struct Neighbours {
        std::array<Vec2i, 8> cells{};
        int count = 0;
        [[nodiscard]] const Vec2i* begin() const noexcept { return cells.data(); }
        [[nodiscard]] const Vec2i* end() const noexcept { return cells.data() + count; }
    };

    Grid() = default;
    Grid(int w, int h, const T& fill = T{}) : w_(w), h_(h), cells_(static_cast<std::size_t>(w) * h, fill) {}

    [[nodiscard]] int width() const noexcept { return w_; }
    [[nodiscard]] int height() const noexcept { return h_; }
    [[nodiscard]] bool contains(Vec2i p) const noexcept { return p.x >= 0 && p.y >= 0 && p.x < w_ && p.y < h_; }

    T& operator()(int x, int y) noexcept { return cells_[index(x, y)]; }
    const T& operator()(int x, int y) const noexcept { return cells_[index(x, y)]; }
    T& operator()(Vec2i p) noexcept { return (*this)(p.x, p.y); }
    const T& operator()(Vec2i p) const noexcept { return (*this)(p.x, p.y); }

    [[nodiscard]] Neighbours neighbours(Vec2i p, bool diagonal = true) const noexcept {
        Neighbours n;
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx) {
                if ((dx == 0 && dy == 0) || (!diagonal && dx != 0 && dy != 0)) continue;
                const Vec2i q{p.x + dx, p.y + dy};
                if (contains(q)) n.cells[static_cast<std::size_t>(n.count++)] = q;
            }
        return n;
    }

    void fill(const T& v) { std::fill(cells_.begin(), cells_.end(), v); }

    auto begin() noexcept { return cells_.begin(); }
    auto end() noexcept { return cells_.end(); }
    auto begin() const noexcept { return cells_.begin(); }
    auto end() const noexcept { return cells_.end(); }

    [[nodiscard]] std::vector<T>& raw() noexcept { return cells_; }
    [[nodiscard]] const std::vector<T>& raw() const noexcept { return cells_; }

private:
    [[nodiscard]] std::size_t index(int x, int y) const noexcept { return static_cast<std::size_t>(y) * w_ + x; }
    int w_ = 0, h_ = 0;
    std::vector<T> cells_;
};

}
