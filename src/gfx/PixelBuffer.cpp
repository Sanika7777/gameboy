#include "gfx/PixelBuffer.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace nb::gfx {

PixelBuffer::PixelBuffer(int w, int h, Shade fillShade)
    : w_(w), h_(h), data_(new Shade[static_cast<std::size_t>(w) * h]) {
    std::fill_n(data_, w_ * h_, fillShade);
}

PixelBuffer::PixelBuffer(const PixelBuffer& other)
    : w_(other.w_), h_(other.h_), data_(other.data_ ? new Shade[static_cast<std::size_t>(w_) * h_] : nullptr) {
    std::copy_n(other.data_, w_ * h_, data_);
}

PixelBuffer::PixelBuffer(PixelBuffer&& other) noexcept
    : w_(std::exchange(other.w_, 0)), h_(std::exchange(other.h_, 0)),
      data_(std::exchange(other.data_, nullptr)) {}

// Copy-and-swap: the copy may throw, but *this is untouched until the no-throw swap.
PixelBuffer& PixelBuffer::operator=(const PixelBuffer& other) {
    PixelBuffer copy(other);
    swap(*this, copy);
    return *this;
}

PixelBuffer& PixelBuffer::operator=(PixelBuffer&& other) noexcept {
    PixelBuffer moved(std::move(other));
    swap(*this, moved);
    return *this;
}

PixelBuffer::~PixelBuffer() { delete[] data_; }

void swap(PixelBuffer& a, PixelBuffer& b) noexcept {
    using std::swap;
    swap(a.w_, b.w_);
    swap(a.h_, b.h_);
    swap(a.data_, b.data_);
}

Shade& PixelBuffer::at(int x, int y) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) throw std::out_of_range("PixelBuffer::at");
    return (*this)(x, y);
}

void PixelBuffer::fill(Shade s) noexcept { std::fill_n(data_, w_ * h_, s); }

bool operator==(const PixelBuffer& a, const PixelBuffer& b) noexcept {
    return a.w_ == b.w_ && a.h_ == b.h_ && std::equal(a.data_, a.data_ + a.w_ * a.h_, b.data_);
}

} // namespace nb::gfx
