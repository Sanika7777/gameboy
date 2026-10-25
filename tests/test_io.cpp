#include "TestHarness.hpp"

#include "io/Binary.hpp"
#include "io/HighScoreTable.hpp"
#include "io/IniFile.hpp"
#include "io/Settings.hpp"

#include <fstream>

using namespace nb::io;

namespace {

std::filesystem::path tmp(const std::string& name) { return std::filesystem::temp_directory_path() / name; }

void writeText(const std::filesystem::path& p, const std::string& s) { std::ofstream(p) << s; }

ScoreEntry entry(char c, std::uint32_t v, std::uint32_t t) { return {{c, c, c}, v, t}; }

} // namespace

TEST("Serializer round trip: ints, floats, strings, containers") {
    BinaryWriter w;
    w << std::uint16_t{0xBEEF} << std::int32_t{-5} << 1.5f << true << std::string("hi")
      << std::vector<int>{1, 2, 3} << std::array<char, 3>{'a', 'b', 'c'};
    CHECK(w.data()[0] == 0xEF);   // little-endian
    BinaryReader r(w.data());
    CHECK(r.get<std::uint16_t>() == 0xBEEF);
    CHECK(r.get<std::int32_t>() == -5);
    CHECK(r.get<float>() == 1.5f);
    CHECK(r.get<bool>());
    CHECK(r.get<std::string>() == "hi");
    CHECK((r.get<std::vector<int>>() == std::vector<int>{1, 2, 3}));
    CHECK((r.get<std::array<char, 3>>() == std::array<char, 3>{'a', 'b', 'c'}));
    CHECK(r.remaining() == 0u);
    CHECK_THROWS_AS(r.get<std::uint8_t>(), nb::CorruptSaveError);
}

TEST("CRC32 known value") {
    const std::string s = "123456789";
    CHECK(crc32({reinterpret_cast<const std::uint8_t*>(s.data()), s.size()}) == 0xCBF43926u);
}

TEST("IniFile round trip and specializations") {
    const auto p = tmp("nb_test.ini");
    writeText(p, "# c\n[a]\nx = 5   # trailing\nflag = true\nname = moss\n[b]\nbad = nope\n");
    const auto ini = IniFile::load(p);
    CHECK(ini.get<int>("a", "x", 0) == 5);
    CHECK(ini.get<bool>("a", "flag", false));
    CHECK(ini.get<std::string>("a", "name", "") == "moss");
    CHECK(ini.get<int>("b", "bad", 7) == 7);
    CHECK(ini.get<int>("zz", "x", 3) == 3);
    writeText(p, ini.text());
    CHECK(IniFile::load(p).get<int>("a", "x", 0) == 5);
}

TEST("IniFile bad line: exact exception, or skipped when tolerant") {
    const auto p = tmp("nb_bad.ini");
    writeText(p, "[a]\nx = 1\nnonsense\ny = 2\n");
    try {
        (void)IniFile::load(p);
        CHECK(false);
    } catch (const nb::ParseError& e) {
        CHECK(e.line() == 3);
    }
    std::vector<nb::ParseError> errors;
    const auto ini = IniFile::load(p, &errors);
    CHECK(errors.size() == 1u);
    CHECK(ini.get<int>("a", "y", 0) == 2);
}

TEST("Settings apply persists and reloads") {
    const auto p = tmp("nb_settings.ini");
    std::filesystem::remove(p);
    NativePersistence persist;
    Settings s(p, persist);
    s.load();
    CHECK(s.get().palette == "moss");
    auto v = s.get();
    v.palette = "ember";
    v.volume = 2;
    v.leftHanded = true;
    s.apply(v);
    Settings again(p, persist);
    again.load();
    CHECK(again.get() == v);
}

TEST("HighScoreTable ordering both directions") {
    HighScoreTable hi(ScoreOrder::HigherIsBetter);
    hi.submit(entry('A', 10, 1));
    hi.submit(entry('B', 30, 2));
    hi.submit(entry('C', 10, 3));
    CHECK(hi.entries()[0].value == 30u);
    CHECK(hi.entries()[1].initials[0] == 'A');   // tie: earlier first
    HighScoreTable lo(ScoreOrder::LowerIsBetter);
    lo.submit(entry('A', 50, 1));
    lo.submit(entry('B', 20, 2));
    CHECK(lo.entries()[0].value == 20u);
    for (std::uint32_t i = 0; i < 12; ++i) hi.submit(entry('Z', 100 + i, 10 + i));
    CHECK(hi.entries().size() == HighScoreTable::kMax);
    CHECK(!hi.qualifies(5));
    CHECK(hi.qualifies(1000));
    CHECK(!hi.submit(entry('Q', 1, 99)).has_value());
}

TEST("ScoreBook round trip and corrupt inputs") {
    NativePersistence persist;
    ScoreBook book(tmp("nb_scores.dat"), persist);
    book.table("serpent.box", ScoreOrder::HigherIsBetter).submit(entry('S', 42, 7));
    book.table("minefield.rookie", ScoreOrder::LowerIsBetter).submit(entry('M', 99, 8));
    const auto bytes = book.encode();

    ScoreBook other(tmp("nb_scores2.dat"), persist);
    other.decode(bytes);
    CHECK(other.find("serpent.box")->entries()[0].value == 42u);

    auto badMagic = bytes;
    badMagic[0] = 'X';
    CHECK_THROWS_AS(other.decode(badMagic), nb::CorruptSaveError);
    auto badCrc = bytes;
    badCrc[10] ^= 0xFF;
    CHECK_THROWS_AS(other.decode(badCrc), nb::CorruptSaveError);
    auto newer = bytes;
    newer[4] = 9;
    CHECK_THROWS_AS(other.decode(newer), nb::VersionMismatchError);
    const std::vector<std::uint8_t> truncated(bytes.begin(), bytes.begin() + 7);
    CHECK_THROWS_AS(other.decode(truncated), nb::CorruptSaveError);
}

TEST("ScoreBook recovers from a corrupt file with a backup") {
    NativePersistence persist;
    const auto p = tmp("nb_corrupt.dat");
    std::filesystem::remove(tmp("nb_corrupt.dat.bak"));
    writeText(p, "garbage garbage");
    ScoreBook book(p, persist);
    const auto toast = book.load();
    CHECK(toast && *toast == "SCORES RESET");
    CHECK(std::filesystem::exists(tmp("nb_corrupt.dat.bak")));
    book.table("x", ScoreOrder::HigherIsBetter).submit(entry('A', 1, 1));
    book.save();
    ScoreBook again(p, persist);
    CHECK(!again.load().has_value());
    CHECK(again.find("x") != nullptr);
}
