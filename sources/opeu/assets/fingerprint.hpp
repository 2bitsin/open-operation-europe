#pragma once
// A 64-bit fingerprint of bytes (FNV-1a): how the renderer knows bytes it has drawn before, and how the content index
// knows the shipped asset a run of bytes is.

#include "opeu/assets/bytes.hpp"

#include <algorithm>
#include <cstdint>

namespace opeu::assets::detail::fingerprint {

class Fingerprint {
public:
  constexpr auto Add(std::uint8_t const byte) noexcept -> void {
    _value = (_value ^ byte) * PRIME;
  }
  constexpr auto Add(Bytes const bytes) noexcept -> void {
    std::ranges::for_each(bytes, [this](std::byte const byte) { Add(std::to_integer<std::uint8_t>(byte)); });
  }
  [[nodiscard]] constexpr auto Value() const noexcept -> std::uint64_t {
    return _value;
  }

private:
  static constexpr auto OFFSET{ std::uint64_t{ 0xCBF29CE484222325 } };
  static constexpr auto PRIME{ std::uint64_t{ 0x00000100000001B3 } };

  std::uint64_t _value{ OFFSET };
};

[[nodiscard]] constexpr auto FingerprintOf(Bytes const bytes) noexcept -> std::uint64_t {
  auto fingerprint{ Fingerprint{} };
  fingerprint.Add(bytes);
  return fingerprint.Value();
}

}

namespace opeu::assets {
using detail::fingerprint::Fingerprint;
using detail::fingerprint::FingerprintOf;
}
