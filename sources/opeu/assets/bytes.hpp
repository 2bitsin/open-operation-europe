#pragma once
// Reading the shipped bytes: little-endian words, bounds-checked slices, fixed-size records as a range, and a
// sequential reader for the coded formats.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <span>

namespace opeu::assets::detail::bytes {

using Bytes = std::span<std::byte const>;

[[nodiscard]] constexpr auto Byte(Bytes const bytes, std::size_t const at) -> std::uint8_t {
  return std::to_integer<std::uint8_t>(bytes[at]);
}

[[nodiscard]] constexpr auto Word(Bytes const bytes, std::size_t const at) -> std::uint16_t {
  return static_cast<std::uint16_t>(Byte(bytes, at) | Byte(bytes, at + 1) << 8);
}

// Bit `pixel` of a byte counted from the left (bit 7 is pixel 0).
[[nodiscard]] constexpr auto PixelBit(std::byte const b, std::size_t const pixel) -> std::uint8_t {
  return static_cast<std::uint8_t>(std::to_integer<unsigned>(b) >> (7 - pixel) & 1);
}

// 0, 1, ..., count - 1.
[[nodiscard]] constexpr auto Indices(std::size_t const count) {
  return std::views::iota(0uz, count);
}

// `count` consecutive records of `size` bytes from the start of `bytes` (which must hold them).
[[nodiscard]] constexpr auto Records(Bytes const bytes, std::size_t const size, std::size_t const count) {
  return Indices(count)
         | std::views::transform([bytes, size](std::size_t const n) { return bytes.subspan(n * size, size); });
}

// As many whole records of `size` bytes as `bytes` holds.
[[nodiscard]] constexpr auto Chunks(Bytes const bytes, std::size_t const size) {
  return Records(bytes, size, bytes.size() / size);
}

// `count` bytes from `at`, or nothing when the span is shorter.
[[nodiscard]] constexpr auto Slice(Bytes const bytes, std::size_t const at, std::size_t const count)
    -> std::optional<Bytes> {
  if (at > bytes.size() || count > bytes.size() - at) return std::nullopt;
  return bytes.subspan(at, count);
}

// The bytes of a coded record read front to back; reading past the end yields zeros and marks it overrun.
class ByteReader {
public:
  constexpr explicit ByteReader(Bytes const bytes) : _bytes{ bytes } { }

  constexpr auto Next() -> std::uint8_t {
    if (_at >= _bytes.size()) {
      _overrun = true;
      return 0;
    }
    return Byte(_bytes, _at++);
  }
  constexpr auto NextWord() -> std::uint16_t {
    auto const low{ Next() };
    return static_cast<std::uint16_t>(low | Next() << 8);
  }
  [[nodiscard]] constexpr auto Position() const noexcept -> std::size_t {
    return _at;
  }
  [[nodiscard]] constexpr auto Overrun() const noexcept -> bool {
    return _overrun;
  }

private:
  Bytes _bytes;
  std::size_t _at{};
  bool _overrun{};
};

}

namespace opeu::assets {
using detail::bytes::Byte;
using detail::bytes::ByteReader;
using detail::bytes::Bytes;
using detail::bytes::Chunks;
using detail::bytes::Indices;
using detail::bytes::PixelBit;
using detail::bytes::Records;
using detail::bytes::Slice;
using detail::bytes::Word;
}
