#include "TestHarness.hpp"

#include "core/Random.hpp"
#include "core/Rect.hpp"
#include "core/Vec2.hpp"

#include <set>
#include <unordered_set>

using nb::Rect;
using nb::Vec2i;

TEST("Vec2 arithmetic and ordering") {
    constexpr Vec2i a{1, 2}, b{3, -4};
    static_assert(a + b == Vec2i{4, -2});
    CHECK(a - b == (Vec2i{-2, 6}));
    CHECK(a * 3 == (Vec2i{3, 6}));
    CHECK(-a == (Vec2i{-1, -2}));
    CHECK(a < b);
    CHECK((Vec2i{1, 3}) > a);
    std::set<Vec2i> s{{2, 2}, {1, 5}, {1, 1}};
    CHECK(*s.begin() == (Vec2i{1, 1}));
    std::unordered_set<Vec2i> u{{1, 1}, {1, 1}, {-1, 1}};
    CHECK(u.size() == 2u);
    std::ostringstream os;
    os << a;
    CHECK(os.str() == "(1, 2)");
}

TEST("Rect contains and intersection") {
    constexpr Rect r(10, 10, 5, 5);
    static_assert(Rect(4, 3) == Rect(0, 0, 4, 3));
    CHECK(r.contains({10, 10}));
    CHECK(!r.contains({15, 10}));
    CHECK((r & Rect(12, 12, 10, 10)) == Rect(12, 12, 3, 3));
    CHECK((r & Rect(0, 0, 5, 5)).empty());
}

TEST("Random is deterministic and state round-trips") {
    nb::Random a(42), b(42);
    for (int i = 0; i < 20; ++i) CHECK(a.range(0, 1000) == b.range(0, 1000));
    const auto saved = a.state();
    const int next = a.range(0, 1 << 20);
    nb::Random c(7);
    c.setState(saved);
    CHECK(c.range(0, 1 << 20) == next);
}
