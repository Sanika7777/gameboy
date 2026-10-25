#include "gfx/Palette.hpp"

#include "core/Error.hpp"
#include "io/TextReader.hpp"

#include <algorithm>
#include <utility>

namespace nb::gfx {

namespace {

struct Registry {
    std::vector<std::string> names{"moss"};
    std::vector<Palette> palettes{Palette{0xFFC9D69A, 0xFF8BA35A, 0xFF3E5C38, 0xFF1E2A1E}};
};

Registry& registry() {
    static Registry r;
    return r;
}

} // namespace

Palette::Palette(std::initializer_list<Rgba> shades) {
    if (shades.size() != 4) throw ContractViolation("Palette needs exactly 4 shades");
    std::copy(shades.begin(), shades.end(), c_.begin());
}

void Palette::loadPresets(const std::filesystem::path& file) {
    io::TextReader r(file);
    Registry loaded{{}, {}};
    while (r.next()) {
        const auto t = r.tokens();
        if (t.size() != 6 || t[0] != "palette") r.fail("expected: palette <name> <4 hex colours>");
        auto hex = [&](std::string_view s) { return 0xFF000000u | static_cast<Rgba>(io::parseInt(r, s, 16)); };
        loaded.names.emplace_back(t[1]);
        loaded.palettes.push_back(Palette{hex(t[2]), hex(t[3]), hex(t[4]), hex(t[5])});
    }
    if (loaded.names.empty()) r.fail("no palettes");
    registry() = std::move(loaded);
}

const Palette& Palette::preset(std::string_view name) {
    const auto& reg = registry();
    const auto it = std::find(reg.names.begin(), reg.names.end(), name);
    return it == reg.names.end() ? reg.palettes.front() : reg.palettes[static_cast<std::size_t>(it - reg.names.begin())];
}

const std::vector<std::string>& Palette::presetNames() { return registry().names; }

} // namespace nb::gfx
