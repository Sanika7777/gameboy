#pragma once

// Role: the user's settings, persisted in settings.ini.

#include "io/IPersistence.hpp"

#include <filesystem>
#include <string>

namespace nb::io {

struct SettingsValues {
    std::string palette = "moss";
    std::string caseTheme = "brick";
    std::string lcdEffect = "none";       // none | ghosting | grid
    int volume = 5;                       // 0-7
    bool haptics = true;
    std::string layout = "auto";          // auto | portrait | landscape
    bool leftHanded = false;
    bool reduceFlashing = false;

    bool operator==(const SettingsValues&) const = default;
};

class Settings {
public:
    Settings(std::filesystem::path file, IPersistence& persistence)
        : file_(std::move(file)), persistence_(persistence) {}

    // Missing file → defaults; bad lines are skipped and logged (design §9.4).
    void load();
    [[nodiscard]] const SettingsValues& get() const noexcept { return values_; }
    // Strong guarantee: the file is written first; values change only if that succeeded.
    void apply(SettingsValues next);

private:
    std::filesystem::path file_;
    IPersistence& persistence_;
    SettingsValues values_;
};

} // namespace nb::io
