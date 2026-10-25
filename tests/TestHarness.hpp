#pragma once

// Role: tiny self-registering test framework (no external test library allowed).

#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <span>
#include <string>
#include <vector>

namespace nbtest {

struct Case {
    const char* name;
    std::function<void()> fn;
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failures() {
    static int n = 0;
    return n;
}

struct Registrar {
    Registrar(const char* name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

inline void fail(const char* file, int line, const std::string& what) {
    ++failures();
    std::cerr << file << ':' << line << ": FAILED: " << what << '\n';
}

// FNV-1a over raw bytes: renderer tests compare buffer hashes.
template<class T>
std::uint64_t hashBytes(std::span<const T> data) {
    std::uint64_t h = 1469598103934665603ull;
    for (const auto& v : data) {
        h ^= static_cast<std::uint64_t>(v);
        h *= 1099511628211ull;
    }
    return h;
}

} // namespace nbtest

#define NB_CAT2(a, b) a##b
#define NB_CAT(a, b) NB_CAT2(a, b)
#define TEST(name)                                                                     \
    static void NB_CAT(test_fn_, __LINE__)();                                          \
    static const nbtest::Registrar NB_CAT(test_reg_, __LINE__){name, &NB_CAT(test_fn_, __LINE__)}; \
    static void NB_CAT(test_fn_, __LINE__)()

#define CHECK(expr)                                                                    \
    do {                                                                               \
        if (!(expr)) nbtest::fail(__FILE__, __LINE__, #expr);                          \
    } while (0)

#define CHECK_EQ(a, b)                                                                 \
    do {                                                                               \
        const auto& nb_a_ = (a);                                                       \
        const auto& nb_b_ = (b);                                                       \
        if (!(nb_a_ == nb_b_)) {                                                       \
            std::ostringstream nb_os_;                                                 \
            nb_os_ << #a " == " #b " (" << nb_a_ << " vs " << nb_b_ << ")";            \
            nbtest::fail(__FILE__, __LINE__, nb_os_.str());                            \
        }                                                                              \
    } while (0)

// Passes only for exactly Type (a more derived exception counts as a failure).
#define CHECK_THROWS_AS(expr, Type)                                                    \
    do {                                                                               \
        bool nb_ok_ = false;                                                           \
        try {                                                                          \
            (void)(expr);                                                              \
        } catch (const Type& e) {                                                      \
            nb_ok_ = typeid(e) == typeid(Type);                                        \
        } catch (...) {                                                                \
        }                                                                              \
        if (!nb_ok_) nbtest::fail(__FILE__, __LINE__, "expected exactly " #Type " from " #expr); \
    } while (0)

#include <sstream>
#include <typeinfo>
