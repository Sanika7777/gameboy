#pragma once

// Role: hands audio commands from the game thread to the audio callback without locks.
// Demonstrates: class template with a non-type parameter; std::array; std::atomic

#include <array>
#include <atomic>
#include <cstddef>

namespace nb {

// Single producer, single consumer. One slot stays empty to tell full from empty.
template<class T, std::size_t N>
class RingBuffer {
public:
    bool push(const T& v) noexcept {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto next = (head + 1) % N;
        if (next == tail_.load(std::memory_order_acquire)) return false;   // full: drop
        buf_[head] = v;
        head_.store(next, std::memory_order_release);
        return true;
    }
    bool pop(T& out) noexcept {
        const auto tail = tail_.load(std::memory_order_relaxed);
        if (tail == head_.load(std::memory_order_acquire)) return false;   // empty
        out = buf_[tail];
        tail_.store((tail + 1) % N, std::memory_order_release);
        return true;
    }
    [[nodiscard]] bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    [[nodiscard]] static constexpr std::size_t capacity() noexcept { return N - 1; }

private:
    std::array<T, N> buf_{};
    std::atomic<std::size_t> head_{0}, tail_{0};
};

} // namespace nb
