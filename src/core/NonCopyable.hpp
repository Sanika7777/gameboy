#pragma once

// Role: mixin that forbids copying of objects that own unique resources.
// Demonstrates: = delete / = default; private inheritance

namespace nb {

class NonCopyable {
protected:
    // Protected: only derived classes can construct one, and a NonCopyable* is never deleted
    // polymorphically, so a non-virtual destructor is correct here.
    NonCopyable() = default;
    ~NonCopyable() = default;

public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

} // namespace nb
