#include "io/TextReader.hpp"

#include <charconv>

namespace nb::io {
namespace {
std::string_view trim(std::string_view s) {
    const auto b = s.find_first_not_of(" \t\r");
    if (b == std::string_view::npos) return {};
    const auto e = s.find_last_not_of(" \t\r");
    return s.substr(b, e - b + 1);
}

}

TextReader::TextReader(const std::filesystem::path& file)
    : in_(file), name_(file.filename().string()) {
    if (!in_) throw AssetError("FILE NOT FOUND", file.string());
}

bool TextReader::nextRaw() {
    if (!std::getline(in_, buf_)) return false;
    ++lineNo_;
    view_ = buf_;
    while (!view_.empty() && (view_.back() == '\r' || view_.back() == ' ' || view_.back() == '\t'))
        view_.remove_suffix(1);
    return true;
}

bool TextReader::next() {
    while (nextRaw()) {
        auto v = trim(view_);
        if (v.empty() || v.front() == '#') continue;
        if (auto c = v.find(" #"); c != std::string_view::npos) v = trim(v.substr(0, c));
        if (auto c = v.find("\t#"); c != std::string_view::npos) v = trim(v.substr(0, c));
        view_ = v;
        return true;
    }
    return false;
}

std::vector<std::string_view> TextReader::tokens() const {
    std::vector<std::string_view> out;
    std::size_t i = 0;
    while (i < view_.size()) {
        while (i < view_.size() && (view_[i] == ' ' || view_[i] == '\t')) ++i;
        const auto start = i;
        while (i < view_.size() && view_[i] != ' ' && view_[i] != '\t') ++i;
        if (i > start) out.push_back(view_.substr(start, i - start));
    }
    return out;
}

void TextReader::fail(const std::string& message) const { throw ParseError(name_, lineNo_, message); }

int parseInt(const TextReader& r, std::string_view s, int base) {
    int v = 0;
    const auto [p, ec] = std::from_chars(s.data(), s.data() + s.size(), v, base);
    if (ec != std::errc{} || p != s.data() + s.size()) r.fail("bad number '" + std::string(s) + "'");
    return v;
}

float parseFloat(const TextReader& r, std::string_view s) {
    try {
        std::size_t used = 0;
        const float v = std::stof(std::string(s), &used);
        if (used == s.size()) return v;
    } catch (const std::exception&) {
    }
    r.fail("bad number '" + std::string(s) + "'");
}

Attrs::Attrs(const TextReader& reader, std::span<const std::string_view> tokens) : reader_(reader) {
    std::vector<std::string_view>* current = nullptr;
    for (auto t : tokens) {
        const auto eq = t.find('=');
        if (eq == std::string_view::npos) {
            if (!current) reader_.fail("expected key=value, got '" + std::string(t) + "'");
            current->push_back(t);
            continue;
        }
        auto value = t.substr(eq + 1);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') value = value.substr(1, value.size() - 2);
        current = &values_[std::string(t.substr(0, eq))];
        current->push_back(value);
    }
}

bool Attrs::has(std::string_view key) const { return values_.find(key) != values_.end(); }

const std::vector<std::string_view>& Attrs::list(std::string_view key) const {
    const auto it = values_.find(key);
    if (it == values_.end()) reader_.fail("missing '" + std::string(key) + "'");
    return it->second;
}

std::string_view Attrs::str(std::string_view key) const { return list(key).front(); }
std::string_view Attrs::str(std::string_view key, std::string_view fallback) const {
    return has(key) ? str(key) : fallback;
}
int Attrs::integer(std::string_view key) const { return parseInt(reader_, str(key)); }
int Attrs::integer(std::string_view key, int fallback) const { return has(key) ? integer(key) : fallback; }
float Attrs::real(std::string_view key) const { return parseFloat(reader_, str(key)); }
float Attrs::real(std::string_view key, float fallback) const { return has(key) ? real(key) : fallback; }

}
