#pragma once

// Role: per-class live-instance counter, mixed into each concrete game.
// Demonstrates: CRTP (static polymorphism); private inheritance

namespace nb {

// Each T gets its own static counter because Counted<A> and Counted<B> are different classes.
template<class T>
class Counted {
public:
    [[nodiscard]] static int alive() noexcept { return count_; }

protected:
    Counted() noexcept { ++count_; }
    Counted(const Counted&) noexcept { ++count_; }
    Counted& operator=(const Counted&) noexcept = default;
    ~Counted() { --count_; }

private:
    static inline int count_ = 0;
};

} // namespace nb
