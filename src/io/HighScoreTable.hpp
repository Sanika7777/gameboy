#pragma once

#include "io/IPersistence.hpp"

#include <array>
#include <compare>
#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace nb::io {
struct ScoreEntry {
    std::array<char, 3> initials{'-', '-', '-'};
    std::uint32_t value = 0;
    std::uint32_t time = 0;

    auto operator<=>(const ScoreEntry& o) const {
        if (auto c = value <=> o.value; c != 0) return c;
        return time <=> o.time;
    }
    bool operator==(const ScoreEntry&) const = default;
};

enum class ScoreOrder : std::uint8_t { HigherIsBetter, LowerIsBetter };

class HighScoreTable {
public:
    static constexpr std::size_t kMax = 10;

    explicit HighScoreTable(ScoreOrder order = ScoreOrder::HigherIsBetter) : order_(order) {}

    [[nodiscard]] const std::vector<ScoreEntry>& entries() const noexcept { return entries_; }
    [[nodiscard]] ScoreOrder order() const noexcept { return order_; }
    void setOrder(ScoreOrder o);
    [[nodiscard]] bool qualifies(std::uint32_t value) const noexcept;
    [[nodiscard]] std::optional<std::uint32_t> best() const noexcept {
        return entries_.empty() ? std::nullopt : std::optional{entries_.front().value};
    }
    std::optional<std::size_t> submit(const ScoreEntry& e);
    void restore(std::vector<ScoreEntry> entries) { entries_ = std::move(entries); }

private:
    [[nodiscard]] bool better(const ScoreEntry& a, const ScoreEntry& b) const noexcept;

    ScoreOrder order_;
    std::vector<ScoreEntry> entries_;
};

class ScoreBook {
public:
    ScoreBook(std::filesystem::path file, IPersistence& persistence) : file_(std::move(file)), persistence_(persistence) {}

    [[nodiscard]] std::optional<std::string> load();
    void save();

    [[nodiscard]] HighScoreTable& table(std::string_view key, ScoreOrder order);
    [[nodiscard]] const HighScoreTable* find(std::string_view key) const;
    [[nodiscard]] std::optional<std::uint32_t> bestFor(std::string_view id, ScoreOrder order) const;
    void clear() { tables_.clear(); }

    [[nodiscard]] std::vector<std::uint8_t> encode() const;
    void decode(std::span<const std::uint8_t> bytes);

private:
    std::filesystem::path file_;
    IPersistence& persistence_;
    bool readOnly_ = false;
    std::map<std::string, HighScoreTable, std::less<>> tables_;
};

}
