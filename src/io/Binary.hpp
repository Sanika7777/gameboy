#pragma once

// Role: little-endian binary encoding for save files, plus CRC32 and atomic file writes.
// Demonstrates: full and partial template specialization (Serializer); operator overloading (<<, >>);
//               binary file I/O; std::filesystem

#include "core/Error.hpp"

#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <type_traits>
#include <vector>

namespace nb::io {

[[nodiscard]] std::uint32_t crc32(std::span<const std::uint8_t> bytes) noexcept;

// Writes to `<path>.tmp` then renames over `path`, so a crash never leaves half a file.
void atomicWrite(const std::filesystem::path& path, std::span<const std::uint8_t> bytes);
[[nodiscard]] std::vector<std::uint8_t> readAll(const std::filesystem::path& path);   // throws FileNotFoundError

class BinaryWriter;
class BinaryReader;

// Primary template: integers, floats, bools and enums, always little-endian on disk.
template<class T>
struct Serializer {
    static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>, "no Serializer for this type");
    static void write(BinaryWriter& w, const T& v);
    static void read(BinaryReader& r, T& v);
};

class BinaryWriter {
public:
    void bytes(std::span<const std::uint8_t> b) { buf_.insert(buf_.end(), b.begin(), b.end()); }
    void byte(std::uint8_t b) { buf_.push_back(b); }

    template<class T>
    BinaryWriter& operator<<(const T& v) {
        Serializer<T>::write(*this, v);
        return *this;
    }

    [[nodiscard]] const std::vector<std::uint8_t>& data() const noexcept { return buf_; }
    // Appends the CRC32 of everything so far.
    void sealWithCrc() { *this << crc32(buf_); }

private:
    std::vector<std::uint8_t> buf_;
};

class BinaryReader {
public:
    explicit BinaryReader(std::span<const std::uint8_t> data) noexcept : data_(data) {}

    std::uint8_t byte() {
        need(1);
        return data_[pos_++];
    }
    std::span<const std::uint8_t> bytes(std::size_t n) {
        need(n);
        auto s = data_.subspan(pos_, n);
        pos_ += n;
        return s;
    }
    template<class T>
    BinaryReader& operator>>(T& v) {
        Serializer<T>::read(*this, v);
        return *this;
    }
    template<class T>
    [[nodiscard]] T get() {
        T v{};
        *this >> v;
        return v;
    }

    [[nodiscard]] std::size_t remaining() const noexcept { return data_.size() - pos_; }

    // Checks the trailing CRC32 and strips it from the readable range.
    void verifyCrc();

private:
    void need(std::size_t n) const {
        if (data_.size() - pos_ < n) throw CorruptSaveError("SAVE TRUNCATED");
    }
    std::span<const std::uint8_t> data_;
    std::size_t pos_ = 0;
};

template<class T>
void Serializer<T>::write(BinaryWriter& w, const T& v) {
    if constexpr (std::is_enum_v<T>) {
        Serializer<std::underlying_type_t<T>>::write(w, static_cast<std::underlying_type_t<T>>(v));
    } else if constexpr (std::is_same_v<T, bool>) {
        w.byte(v ? 1 : 0);
    } else if constexpr (std::is_floating_point_v<T>) {
        static_assert(sizeof(T) == 4 || sizeof(T) == 8);
        using U = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
        Serializer<U>::write(w, std::bit_cast<U>(v));
    } else {
        using U = std::make_unsigned_t<T>;
        const U u = static_cast<U>(v);
        for (std::size_t i = 0; i < sizeof(T); ++i) w.byte(static_cast<std::uint8_t>(u >> (8 * i)));
    }
}

template<class T>
void Serializer<T>::read(BinaryReader& r, T& v) {
    if constexpr (std::is_enum_v<T>) {
        std::underlying_type_t<T> u{};
        Serializer<std::underlying_type_t<T>>::read(r, u);
        v = static_cast<T>(u);
    } else if constexpr (std::is_same_v<T, bool>) {
        v = r.byte() != 0;
    } else if constexpr (std::is_floating_point_v<T>) {
        using U = std::conditional_t<sizeof(T) == 4, std::uint32_t, std::uint64_t>;
        U u{};
        Serializer<U>::read(r, u);
        v = std::bit_cast<T>(u);
    } else {
        using U = std::make_unsigned_t<T>;
        U u = 0;
        for (std::size_t i = 0; i < sizeof(T); ++i) u = static_cast<U>(u | (static_cast<U>(r.byte()) << (8 * i)));
        v = static_cast<T>(u);
    }
}

// Full specialization: length-prefixed string.
template<>
struct Serializer<std::string> {
    static void write(BinaryWriter& w, const std::string& s) {
        w << static_cast<std::uint32_t>(s.size());
        w.bytes({reinterpret_cast<const std::uint8_t*>(s.data()), s.size()});
    }
    static void read(BinaryReader& r, std::string& s) {
        const auto n = r.get<std::uint32_t>();
        const auto b = r.bytes(n);
        s.assign(b.begin(), b.end());
    }
};

// Partial specializations: containers of anything serializable.
template<class T>
struct Serializer<std::vector<T>> {
    static void write(BinaryWriter& w, const std::vector<T>& v) {
        w << static_cast<std::uint32_t>(v.size());
        for (const auto& e : v) w << e;
    }
    static void read(BinaryReader& r, std::vector<T>& v) {
        const auto n = r.get<std::uint32_t>();
        if (n > r.remaining()) throw CorruptSaveError("SAVE TRUNCATED");   // no huge bogus allocations
        v.resize(n);
        for (auto& e : v) r >> e;
    }
};

template<class T, std::size_t N>
struct Serializer<std::array<T, N>> {
    static void write(BinaryWriter& w, const std::array<T, N>& a) {
        for (const auto& e : a) w << e;
    }
    static void read(BinaryReader& r, std::array<T, N>& a) {
        for (auto& e : a) r >> e;
    }
};

} // namespace nb::io
