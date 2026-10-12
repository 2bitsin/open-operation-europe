// Byte and string helpers over the opening's 64 KiB data segment, addressed by near pointers (16-bit offsets). Every
// offset wraps at the end of the segment, as a near pointer does: a copy or a string that runs past FFFFh continues
// at 0000h, and a string's length counts modulo 65536.
#pragma once

#include "opeu/contract.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <span>

namespace opeu::intro::detail::segment_bytes {

inline constexpr std::size_t SEGMENT_SIZE{ 0x10000 };

// A 16-bit word: a near pointer into the segment, or a byte count within it.
using NearWord     = std::uint16_t;
using Segment      = std::span<std::uint8_t, SEGMENT_SIZE>;
using ConstSegment = std::span<std::uint8_t const, SEGMENT_SIZE>;

// A block move inside the segment: `size` bytes from near `source` to near `destination`.
struct NearCopy {
  NearWord source;
  NearWord destination;
  NearWord size;
};

// Moves the block one byte at a time, upwards when the destination lies below the source and downwards otherwise, so
// an overlapping block arrives intact; each byte's offsets wrap within the segment.
inline auto CopyBytes(Segment segment, NearCopy copy) -> void {
  auto const move_byte{ [&](NearWord step) { segment[static_cast<NearWord>(copy.destination + step)] = segment[static_cast<NearWord>(copy.source + step)]; } };
  auto const steps{ std::views::iota(NearWord{ 0 }, copy.size) };
  if (copy.destination < copy.source) {
    std::ranges::for_each(steps, move_byte);
  } else {
    std::ranges::for_each(steps | std::views::reverse, move_byte);
  }
}

// The number of bytes before the terminating zero of the string at near `string`, the search wrapping past the end of
// the segment; a segment without a zero byte has no string end.
[[nodiscard]] inline auto StringLength(ConstSegment segment, NearWord string) -> NearWord {
  auto const tail{ segment.subspan(string) };
  if (auto const end{ std::ranges::find(tail, std::uint8_t{ 0 }) }; end != tail.end()) {
    return static_cast<NearWord>(std::distance(tail.begin(), end));
  }
  auto const head{ segment.first(string) };
  auto const end{ std::ranges::find(head, std::uint8_t{ 0 }) };
  Expects(end != head.end(), "the segment holds a string terminator");
  return static_cast<NearWord>(tail.size() + static_cast<std::size_t>(std::distance(head.begin(), end)));
}

// The ASCII letter in upper case; every other code as it is.
[[nodiscard]] constexpr auto AsciiUpper(std::uint8_t code) -> std::uint8_t {
  if (code >= 'a' && code <= 'z') return static_cast<std::uint8_t>(code - ('a' - 'A'));
  return code;
}

} // namespace opeu::intro::detail::segment_bytes

namespace opeu::intro {
using detail::segment_bytes::AsciiUpper;
using detail::segment_bytes::ConstSegment;
using detail::segment_bytes::CopyBytes;
using detail::segment_bytes::NearCopy;
using detail::segment_bytes::NearWord;
using detail::segment_bytes::Segment;
using detail::segment_bytes::SEGMENT_SIZE;
using detail::segment_bytes::StringLength;
} // namespace opeu::intro
