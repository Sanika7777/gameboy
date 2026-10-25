#include "io/IniFile.hpp"

#include "io/TextReader.hpp"

#include <charconv>
#include <sstream>

namespace nb::io {
namespace {
std::string trimmed(std::string_view s) {
    const auto b = s.find_first_not_of(" \t");
    if (b == std::string_view::npos) return {};
    return std::string(s.substr(b, s.find_last_not_of(" \t") - b + 1));
}

}

IniFile IniFile::load(const std::filesystem::path& file, std::vector<ParseError>* errors) {
    TextReader r(file);
    IniFile ini;
    std::string section;
    while (r.next()) {
        try {
            const auto line = r.line();
            if (line.front() == '[') {
                if (line.back() != ']' || line.size() < 3) r.fail("bad section header");
                section = trimmed(line.substr(1, line.size() - 2));
                continue;
            }
            const auto eq = line.find('=');
            if (eq == std::string_view::npos) r.fail("expected key = value");
            if (section.empty()) r.fail("key outside a [section]");
            const auto key = trimmed(line.substr(0, eq));
            if (key.empty()) r.fail("empty key");
            ini.sections_[section][key] = trimmed(line.substr(eq + 1));
        } catch (const ParseError& e) {
            if (!errors) throw;
            errors->push_back(e);
        }
    }
    return ini;
}

std::string IniFile::text() const {
    std::ostringstream os;
    os << "# NEOBRICK settings\n";
    for (const auto& [name, keys] : sections_) {
        os << '[' << name << "]\n";
        for (const auto& [k, v] : keys) os << k << " = " << v << '\n';
    }
    return os.str();
}

const std::string* IniFile::find(const std::string& section, const std::string& key) const {
    const auto s = sections_.find(section);
    if (s == sections_.end()) return nullptr;
    const auto k = s->second.find(key);
    return k == s->second.end() ? nullptr : &k->second;
}

template<>
std::string IniFile::get<std::string>(const std::string& section, const std::string& key, std::string fallback) const {
    const auto* v = find(section, key);
    return v ? *v : fallback;
}

template<>
int IniFile::get<int>(const std::string& section, const std::string& key, int fallback) const {
    const auto* v = find(section, key);
    if (!v) return fallback;
    int out = 0;
    const auto [p, ec] = std::from_chars(v->data(), v->data() + v->size(), out);
    return (ec == std::errc{} && p == v->data() + v->size()) ? out : fallback;
}

template<>
bool IniFile::get<bool>(const std::string& section, const std::string& key, bool fallback) const {
    const auto* v = find(section, key);
    if (!v) return fallback;
    if (*v == "true" || *v == "on" || *v == "1") return true;
    if (*v == "false" || *v == "off" || *v == "0") return false;
    return fallback;
}

}
