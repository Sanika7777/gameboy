#include "io/Settings.hpp"

#include "core/Log.hpp"
#include "io/Binary.hpp"
#include "io/IniFile.hpp"

#include <algorithm>

namespace nb::io {
void Settings::load() {
    if (!std::filesystem::exists(file_)) return;
    std::vector<ParseError> errors;
    const auto ini = IniFile::load(file_, &errors);
    for (const auto& e : errors) logWarn(e.file(), ':', e.line(), ": ", e.context(), " (using default)");
    SettingsValues v;
    v.palette = ini.get<std::string>("display", "palette", v.palette);
    v.caseTheme = ini.get<std::string>("display", "case", v.caseTheme);
    v.lcdEffect = ini.get<std::string>("display", "lcd_effect", v.lcdEffect);
    v.reduceFlashing = ini.get<bool>("display", "reduce_flashing", v.reduceFlashing);
    v.volume = std::clamp(ini.get<int>("audio", "volume", v.volume), 0, 7);
    v.haptics = ini.get<bool>("input", "haptics", v.haptics);
    v.layout = ini.get<std::string>("input", "layout", v.layout);
    v.leftHanded = ini.get<bool>("input", "left_handed", v.leftHanded);
    values_ = std::move(v);
}

void Settings::apply(SettingsValues next) {
    IniFile ini;
    auto b = [](bool on) { return std::string(on ? "true" : "false"); };
    ini.set("display", "palette", next.palette);
    ini.set("display", "case", next.caseTheme);
    ini.set("display", "lcd_effect", next.lcdEffect);
    ini.set("display", "reduce_flashing", b(next.reduceFlashing));
    ini.set("audio", "volume", std::to_string(next.volume));
    ini.set("input", "haptics", b(next.haptics));
    ini.set("input", "layout", next.layout);
    ini.set("input", "left_handed", b(next.leftHanded));
    const auto text = ini.text();
    atomicWrite(file_, {reinterpret_cast<const std::uint8_t*>(text.data()), text.size()});
    persistence_.commit(file_);
    values_ = std::move(next);
}

}
