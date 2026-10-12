// Text of the data segment: its zero-terminated strings as views, and the formatter behind the game's sprintf and
// printf (core/textout), which builds the text as a C++ string before it is stored or printed.
#pragma once

#include "common/data-segment.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace opeu::game::detail::text {

// The zero-terminated string of a character array (the whole array when it has no terminator).
[[nodiscard]] inline auto CString(std::span<char const> characters) noexcept -> std::string_view {
  return { characters.begin(), std::ranges::find(characters, '\0') };
}

// The zero-terminated string at a near pointer (up to the end of the segment when it has no terminator).
[[nodiscard]] inline auto CString(DataSegment const& data, std::uint16_t near) noexcept -> std::string_view {
  auto const segment{
    std::span{ reinterpret_cast<char const*>(&data), sizeof(DataSegment) }
  };
  return CString(segment.subspan(near));
}

// The game's printf formatting of argument words: %[-][+][0][width][.precision][l](d|u|x|o|s|c), case insensitive,
// digits upper case; %s takes the near pointer of a string, %ld two words (low, high); any other character after % is
// copied as it is. A word the format asks for beyond the list reads as 0.
[[nodiscard]] auto FormatText(DataSegment const& data, std::string_view format, std::span<std::uint16_t const> words) -> std::string;

// Writes the text with its terminator at a near pointer of the data segment, for the texts the segment keeps.
inline auto StoreText(DataSegment& data, std::uint16_t destination, std::string_view text) noexcept -> void {
  auto const segment{
    std::span{ reinterpret_cast<char*>(&data), sizeof(DataSegment) }
  };
  auto const end{ std::ranges::copy(text, segment.subspan(destination).begin()).out };
  *end = '\0';
}

} // namespace opeu::game::detail::text

namespace opeu::game {
using detail::text::CString;
using detail::text::FormatText;
using detail::text::StoreText;
} // namespace opeu::game
