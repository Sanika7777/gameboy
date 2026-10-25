#pragma once

// Role: [section] key = value text files (settings.ini).
// Demonstrates: member function template with full specializations; std::map

#include "core/Error.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace nb::io {

class IniFile {
public:
    // Strict when `errors` is null (throws ParseError); otherwise bad lines are skipped and reported.
    static IniFile load(const std::filesystem::path& file, std::vector<ParseError>* errors = nullptr);
    [[nodiscard]] std::string text() const;

    template<class T>
    [[nodiscard]] T get(const std::string& section, const std::string& key, T fallback) const;

    void set(const std::string& section, const std::string& key, std::string value) {
        sections_[section][key] = std::move(value);
    }

private:
    [[nodiscard]] const std::string* find(const std::string& section, const std::string& key) const;

    // Ordered maps so the written file is stable and diff-friendly.
    std::map<std::string, std::map<std::string, std::string>> sections_;
};

template<>
std::string IniFile::get<std::string>(const std::string& section, const std::string& key, std::string fallback) const;
template<>
int IniFile::get<int>(const std::string& section, const std::string& key, int fallback) const;
template<>
bool IniFile::get<bool>(const std::string& section, const std::string& key, bool fallback) const;

} // namespace nb::io
