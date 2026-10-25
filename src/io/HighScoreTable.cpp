#include "io/HighScoreTable.hpp"

#include "core/Log.hpp"
#include "io/Binary.hpp"

#include <algorithm>

namespace nb::io {

namespace {

constexpr std::array<std::uint8_t, 4> kMagic{'N', 'B', 'H', 'S'};
constexpr std::uint16_t kVersion = 1;

} // namespace

bool HighScoreTable::better(const ScoreEntry& a, const ScoreEntry& b) const noexcept {
    // Value only: std::stable_sort keeps earlier submissions ahead on ties.
    return order_ == ScoreOrder::HigherIsBetter ? a.value > b.value : a.value < b.value;
}

void HighScoreTable::setOrder(ScoreOrder o) {
    if (o == order_) return;
    order_ = o;
    std::stable_sort(entries_.begin(), entries_.end(), [this](const auto& a, const auto& b) { return better(a, b); });
}

bool HighScoreTable::qualifies(std::uint32_t value) const noexcept {
    if (order_ == ScoreOrder::HigherIsBetter && value == 0) return false;   // nothing to be proud of
    if (entries_.size() < kMax) return true;
    ScoreEntry probe;
    probe.value = value;
    return better(probe, entries_.back());
}

std::optional<std::size_t> HighScoreTable::submit(const ScoreEntry& e) {
    auto next = entries_;   // may throw: entries_ untouched
    next.push_back(e);
    std::stable_sort(next.begin(), next.end(), [this](const auto& a, const auto& b) { return better(a, b); });
    if (next.size() > kMax) next.resize(kMax);
    const auto it = std::find(next.begin(), next.end(), e);
    if (it == next.end()) return std::nullopt;
    const auto rank = static_cast<std::size_t>(it - next.begin());
    entries_.swap(next);    // no-throw commit
    return rank;
}

HighScoreTable& ScoreBook::table(std::string_view key, ScoreOrder order) {
    auto it = tables_.find(key);
    if (it == tables_.end()) it = tables_.emplace(std::string(key), HighScoreTable(order)).first;
    it->second.setOrder(order);
    return it->second;
}

const HighScoreTable* ScoreBook::find(std::string_view key) const {
    const auto it = tables_.find(key);
    return it == tables_.end() ? nullptr : &it->second;
}

std::optional<std::uint32_t> ScoreBook::bestFor(std::string_view id, ScoreOrder order) const {
    std::optional<std::uint32_t> best;
    for (const auto& [key, t] : tables_) {
        const bool match = key == id || (key.size() > id.size() && key.starts_with(id) && key[id.size()] == '.');
        if (!match || t.entries().empty()) continue;
        // Recompute from entries: tables loaded from disk don't know their order yet.
        for (const auto& e : t.entries())
            if (!best || (order == ScoreOrder::HigherIsBetter ? e.value > *best : e.value < *best)) best = e.value;
    }
    return best;
}

std::vector<std::uint8_t> ScoreBook::encode() const {
    BinaryWriter w;
    w.bytes(kMagic);
    w << kVersion << static_cast<std::uint16_t>(tables_.size());
    for (const auto& [key, t] : tables_) {
        w.byte(static_cast<std::uint8_t>(std::min<std::size_t>(key.size(), 255)));
        w.bytes({reinterpret_cast<const std::uint8_t*>(key.data()), std::min<std::size_t>(key.size(), 255)});
        w.byte(static_cast<std::uint8_t>(t.entries().size()));
        for (const auto& e : t.entries()) w << e.initials << e.value << e.time;
    }
    w.sealWithCrc();
    return w.data();
}

void ScoreBook::decode(std::span<const std::uint8_t> bytes) {
    BinaryReader r(bytes);
    if (bytes.size() < 8 || !std::equal(kMagic.begin(), kMagic.end(), bytes.begin()))
        throw CorruptSaveError("SCORES CORRUPT", "bad magic");
    r.bytes(4);
    if (r.get<std::uint16_t>() > kVersion) throw VersionMismatchError("SAVE FROM NEWER VERSION");
    r = BinaryReader(bytes);
    r.verifyCrc();
    r.bytes(6);
    decltype(tables_) loaded;
    const auto count = r.get<std::uint16_t>();
    for (int i = 0; i < count; ++i) {
        const auto keyBytes = r.bytes(r.byte());
        std::string key(keyBytes.begin(), keyBytes.end());
        const auto n = r.byte();
        if (n > HighScoreTable::kMax) throw CorruptSaveError("SCORES CORRUPT", "too many entries");
        std::vector<ScoreEntry> entries(n);
        for (auto& e : entries) r >> e.initials >> e.value >> e.time;
        HighScoreTable t;
        t.restore(std::move(entries));
        loaded.emplace(std::move(key), std::move(t));
    }
    if (r.remaining() != 0) throw CorruptSaveError("SCORES CORRUPT", "trailing bytes");
    tables_ = std::move(loaded);
}

std::optional<std::string> ScoreBook::load() {
    if (!std::filesystem::exists(file_)) return std::nullopt;
    try {
        decode(readAll(file_));
        return std::nullopt;
    } catch (const VersionMismatchError& e) {
        readOnly_ = true;
        logWarn("scores.dat: ", e.what());
        return std::string("SAVE FROM NEWER VERSION");
    } catch (const SaveError& e) {
        logWarn("scores.dat: ", e.what(), " ", e.context(), " -> backed up, starting fresh");
        auto bak = file_;
        bak += ".bak";
        std::error_code ec;
        std::filesystem::rename(file_, bak, ec);
        persistence_.commit(bak);
        persistence_.remove(file_);
        tables_.clear();
        return std::string("SCORES RESET");
    }
}

void ScoreBook::save() {
    if (readOnly_) return;
    atomicWrite(file_, encode());
    persistence_.commit(file_);
}

} // namespace nb::io
