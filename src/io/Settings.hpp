#pragma once

#include "io/IPersistence.hpp"

#include <filesystem>
#include <string>

namespace nb::io {
struct SettingsValues {
    std::string palette = "moss";
    std::string caseTheme = "brick";
    std::string lcdEffect = "none";
    int volume = 5;
    bool haptics = true;
    std::string layout = "auto";
    bool leftHanded = false;
    bool reduceFlashing = false;

    bool operator==(const SettingsValues&) const = default;
};

class Settings {
public:
    Settings(std::filesystem::path file, IPersistence& persistence)
        : file_(std::move(file)), persistence_(persistence) {}

    void load();
    [[nodiscard]] const SettingsValues& get() const noexcept { return values_; }
    void apply(SettingsValues next);

private:
    std::filesystem::path file_;
    IPersistence& persistence_;
    SettingsValues values_;
};

}
