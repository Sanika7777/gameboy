#pragma once

// Role: owning 2-D array of shades: the LCD and every sprite.
// Demonstrates: Rule of Five (copy-and-swap, noexcept moves); RAII; friend function; classes and encapsulation

#include "gfx/Shade.hpp"

#include <span>

namespace nb::gfx {

// The one deliberate Rule-of-Five class: it owns a raw array so the copy/move/destructor
// mechanics are visible. RgbaSurface does the same job with std::vector and Rule of Zero.
class PixelBuffer {
public:
    PixelBuffer() noexcept = default;
    PixelBuffer(int w, int h, Shade fill = Shade::Paper);
    PixelBuffer(const PixelBuffer& other);
    PixelBuffer(PixelBuffer&& other) noexcept;
    PixelBuffer& operator=(const PixelBuffer& other);
    PixelBuffer& operator=(PixelBuffer&& other) noexcept;
    ~PixelBuffer();
    friend void swap(PixelBuffer& a, PixelBuffer& b) noexcept;

    [[nodiscard]] int width() const noexcept { return w_; }
    [[nodiscard]] int height() const noexcept { return h_; }
    Shade& operator()(int x, int y) noexcept { return data_[y * w_ + x]; }
    Shade operator()(int x, int y) const noexcept { return data_[y * w_ + x]; }
    Shade& at(int x, int y);                         // throws std::out_of_range
    [[nodiscard]] std::span<Shade> row(int y) noexcept { return {data_ + y * w_, static_cast<std::size_t>(w_)}; }
    [[nodiscard]] std::span<const Shade> pixels() const noexcept {
        return {data_, static_cast<std::size_t>(w_ * h_)};
    }
    void fill(Shade s) noexcept;

    friend bool operator==(const PixelBuffer& a, const PixelBuffer& b) noexcept;

private:
    int w_ = 0, h_ = 0;
    Shade* data_ = nullptr;
};

} // namespace nb::gfx
