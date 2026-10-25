#pragma once

// Role: line-oriented reader for every text asset; knows file name and line number for errors.
// Demonstrates: text file I/O (ifstream, getline); string_view tokenizing

#include "core/Error.hpp"

#include <filesystem>
#include <fstream>
#include <map>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace nb::io {

class TextReader {
public:
    explicit TextReader(const std::filesystem::path& file);   // throws FileNotFoundError

    // Next meaningful line: skips blanks and '#' comment lines, strips " # trailing" comments.
    bool next();
    // Next line as-is (rows of glyphs, sprites and mazes use '#' as data).
    bool nextRaw();

    [[nodiscard]] std::string_view line() const noexcept { return view_; }
    [[nodiscard]] int lineNo() const noexcept { return lineNo_; }
    [[nodiscard]] const std::string& fileName() const noexcept { return name_; }
    [[nodiscard]] std::vector<std::string_view> tokens() const;

    [[noreturn]] void fail(const std::string& message) const;

private:
    std::ifstream in_;
    std::string name_;
    std::string buf_;
    std::string_view view_;
    int lineNo_ = 0;
};

// `key=value` attributes on one line. Bare tokens continue the previous key's value list,
// so `points=4,0 4,250` yields two values for "points".
class Attrs {
public:
    Attrs(const TextReader& reader, std::span<const std::string_view> tokens);

    [[nodiscard]] bool has(std::string_view key) const;
    [[nodiscard]] std::string_view str(std::string_view key) const;          // throws ParseError if missing
    [[nodiscard]] std::string_view str(std::string_view key, std::string_view fallback) const;
    [[nodiscard]] int integer(std::string_view key) const;
    [[nodiscard]] int integer(std::string_view key, int fallback) const;
    [[nodiscard]] float real(std::string_view key) const;
    [[nodiscard]] float real(std::string_view key, float fallback) const;
    [[nodiscard]] const std::vector<std::string_view>& list(std::string_view key) const;

private:
    const TextReader& reader_;
    // Ordered map with transparent comparator: lookups by string_view without allocating.
    std::map<std::string, std::vector<std::string_view>, std::less<>> values_;
};

[[nodiscard]] int parseInt(const TextReader& r, std::string_view s, int base = 10);
[[nodiscard]] float parseFloat(const TextReader& r, std::string_view s);

} // namespace nb::io
