#pragma once

#include "gfx/RgbaSurface.hpp"
#include "gfx/Shade.hpp"

#include <array>
#include <filesystem>
#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

namespace nb::gfx {
class Palette {
public:
    Palette(std::initializer_list<Rgba> shades);

    [[nodiscard]] Rgba operator[](Shade s) const noexcept { return c_[static_cast<std::size_t>(s) & 3]; }

    static void loadPresets(const std::filesystem::path& file);
    [[nodiscard]] static const Palette& preset(std::string_view name);
    [[nodiscard]] static const std::vector<std::string>& presetNames();

private:
    std::array<Rgba, 4> c_{};
};

}
