#include "scenes/SaveState.hpp"

#include "io/Paths.hpp"

#include <algorithm>

namespace nb::scenes {
namespace {
constexpr std::array<std::uint8_t, 4> kMagic{'N', 'B', 'S', 'V'};
constexpr std::uint16_t kVersion = 1;

std::filesystem::path pathFor(std::string_view id) { return io::Paths::save(std::string(id) + ".sav"); }

}

bool hasSave(std::string_view gameId) { return std::filesystem::exists(pathFor(gameId)); }

void saveGame(const games::Game& g, io::IPersistence& p) {
    io::BinaryWriter w;
    w.bytes(kMagic);
    w << kVersion << g.info().id << g.saveVersion();
    g.save(w);
    w.sealWithCrc();
    const auto path = pathFor(g.info().id);
    io::atomicWrite(path, w.data());
    p.commit(path);
}

void loadGame(games::Game& g) {
    const auto bytes = io::readAll(pathFor(g.info().id));
    if (bytes.size() < 4 || !std::equal(kMagic.begin(), kMagic.end(), bytes.begin()))
        throw CorruptSaveError("SAVE CORRUPT", "bad magic");
    io::BinaryReader r(bytes);
    r.verifyCrc();
    r.bytes(4);
    if (r.get<std::uint16_t>() != kVersion) throw VersionMismatchError("SAVE FROM NEWER VERSION");
    if (r.get<std::string>() != g.info().id) throw CorruptSaveError("SAVE CORRUPT", "wrong game");
    if (r.get<std::uint16_t>() != g.saveVersion()) throw VersionMismatchError("SAVE FROM NEWER VERSION");
    g.load(r);
    if (r.remaining() != 0) throw CorruptSaveError("SAVE CORRUPT", "trailing bytes");
}

void deleteSave(std::string_view gameId, io::IPersistence& p) noexcept {
    std::error_code ec;
    const auto path = pathFor(gameId);
    if (std::filesystem::remove(path, ec)) p.remove(path);
}

}
