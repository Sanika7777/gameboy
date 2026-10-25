#include "io/Binary.hpp"

#include <fstream>
#include <iterator>

namespace nb::io {
namespace {
constexpr std::array<std::uint32_t, 256> makeCrcTable() {
    std::array<std::uint32_t, 256> t{};
    for (std::uint32_t i = 0; i < 256; ++i) {
        std::uint32_t c = i;
        for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
        t[i] = c;
    }
    return t;
}

constexpr auto kCrcTable = makeCrcTable();

}

std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept {
    std::uint32_t c = 0xFFFFFFFFu;
    for (const auto b : bytes) c = kCrcTable[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

void atomicWrite(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) {
    auto tmp = path;
    tmp += ".tmp";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        if (!out) throw SaveError("CAN'T WRITE SAVE", tmp.string());
        out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        out.close();
        if (!out) throw SaveError("CAN'T WRITE SAVE", tmp.string());
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) throw SaveError("CAN'T WRITE SAVE", path.string() + ": " + ec.message());
}

std::vector<std::uint8_t> readAll(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw AssetError("FILE NOT FOUND", path.string());
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

void BinaryReader::verifyCrc() {
    if (data_.size() < 4) throw CorruptSaveError("SAVE TRUNCATED");
    const auto body = data_.first(data_.size() - 4);
    BinaryReader tail(data_.last(4));
    if (tail.get<std::uint32_t>() != crc32(body)) throw CorruptSaveError("SAVE CORRUPT", "bad CRC");
    data_ = body;
}

}
