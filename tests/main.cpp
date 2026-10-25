#include "TestHarness.hpp"

int main() {
    for (const auto& c : nbtest::registry()) {
        const int before = nbtest::failures();
        try {
            c.fn();
        } catch (const std::exception& e) {
            nbtest::fail(c.name, 0, std::string("unexpected exception: ") + e.what());
        }
        std::cout << (nbtest::failures() == before ? "  ok   " : "  FAIL ") << c.name << '\n';
    }
    std::cout << nbtest::registry().size() << " tests, " << nbtest::failures() << " failures\n";
    return nbtest::failures() == 0 ? 0 : 1;
}
