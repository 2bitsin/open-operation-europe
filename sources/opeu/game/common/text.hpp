// The game's texts: the zero-terminated strings the data segment keeps at near addresses, the buffers a routine owns
// for the texts it builds, and the formatter behind the game's sprintf and printf (core/textout), which builds the text
// as a C++ string before it is stored or printed.
//
// Through M2 the data segment's texts stay where they are and routines name them by near address, so a routine that
// reads a text takes a Text (a near address or characters it is handed) and one that writes a text a TextTarget (a
// near address or a buffer); M4 replaces the near addresses with typed text tables.
#pragma once

#include "common/data-segment.hpp"

#include "opeu/contract.hpp"

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

namespace opeu::game::detail::text {

// The zero-terminated string of a character array (the whole array when it has no terminator).
[[nodiscard]] inline auto CString(std::span<char const> characters) noexcept -> std::string_view {
  return { characters.begin(), std::ranges::find(characters, '\0') };
}

// The data segment's characters from a near address to its end.
[[nodiscard]] inline auto SegmentCharacters(DataSegment const& data, std::uint16_t near) noexcept -> std::span<char const> {
  return std::span{ reinterpret_cast<char const*>(&data), sizeof(DataSegment) }.subspan(near); // NOLINT(*-reinterpret-cast): the records' bytes
}
[[nodiscard]] inline auto SegmentCharacters(DataSegment& data, std::uint16_t near) noexcept -> std::span<char> {
  return std::span{ reinterpret_cast<char*>(&data), sizeof(DataSegment) }.subspan(near); // NOLINT(*-reinterpret-cast): the records' bytes
}

// The zero-terminated string at a near pointer (up to the end of the segment when it has no terminator).
[[nodiscard]] inline auto CString(DataSegment const& data, std::uint16_t near) noexcept -> std::string_view {
  return CString(SegmentCharacters(data, near));
}

// A text a routine reads: the string at a near address of the data segment, or characters the routine is handed (a
// TextBuffer, a string view). It is read when the routine reads it, so a buffer filled after the Text was taken reads
// as filled.
class Text {
public:
  Text(std::uint16_t near) noexcept : _near{ near } { }                           // NOLINT(misc-explicit-constructor): a near text
  Text(std::span<char const> characters) noexcept : _characters{ characters } { } // NOLINT(misc-explicit-constructor): characters
  Text(std::span<char> characters) noexcept : _characters{ characters } { }       // NOLINT(misc-explicit-constructor): characters
  Text(std::string_view text) noexcept : _characters{ std::span{ text } } { }     // NOLINT(misc-explicit-constructor): characters

  // The string up to its terminator.
  [[nodiscard]] auto View(DataSegment const& data) const noexcept -> std::string_view {
    return CString(Characters(data));
  }
  // The characters from the text's start to the end of its storage: for the routines that read a text by count.
  [[nodiscard]] auto Characters(DataSegment const& data) const noexcept -> std::span<char const> {
    return _characters ? *_characters : SegmentCharacters(data, _near);
  }
  // The near address of a text the data segment keeps: for the routines that store a text's address in the segment.
  [[nodiscard]] auto Near() const -> std::uint16_t {
    Expects(!_characters, "a text stored by its near address lies in the data segment");
    return _near;
  }

private:
  std::uint16_t                        _near{ 0 };
  std::optional<std::span<char const>> _characters;
};

// Where a routine writes a text with its terminator: the data segment from a near address on, or a buffer.
class TextTarget {
public:
  TextTarget(std::uint16_t near) noexcept : _near{ near } { }                     // NOLINT(misc-explicit-constructor): a near buffer
  TextTarget(std::span<char> characters) noexcept : _characters{ characters } { } // NOLINT(misc-explicit-constructor): a buffer

  [[nodiscard]] auto Characters(DataSegment& data) const noexcept -> std::span<char> {
    return _characters ? *_characters : SegmentCharacters(data, _near);
  }
  // The text written there, to read back.
  [[nodiscard]] auto Written() const noexcept -> Text {
    return _characters ? Text{ std::span<char const>{ *_characters } } : Text{ _near };
  }
  operator Text() const noexcept { // NOLINT(misc-explicit-constructor): a target is read back as a text
    return Written();
  }

private:
  std::uint16_t                  _near{ 0 };
  std::optional<std::span<char>> _characters;
};

// The characters of a text a routine builds: zeroed when the routine starts, at most SIZE bytes with the terminator.
template <std::size_t SIZE>
class TextBuffer {
public:
  [[nodiscard]] auto Characters() noexcept -> std::span<char, SIZE> {
    return _characters;
  }
  [[nodiscard]] auto View() const noexcept -> std::string_view {
    return CString(_characters);
  }
  operator Text() const noexcept { // NOLINT(misc-explicit-constructor): a buffer is read as a text
    return Text{ std::span<char const>{ _characters } };
  }
  operator TextTarget() noexcept { // NOLINT(misc-explicit-constructor): a buffer is written as a text
    return TextTarget{ std::span<char>{ _characters } };
  }
  // The text `offset` bytes into the buffer, for a routine whose texts share one buffer as they shared its frame in the
  // original: a text that runs long runs into the next as it did there.
  [[nodiscard]] auto At(std::size_t offset) noexcept -> TextTarget {
    return TextTarget{ std::span<char>{ _characters }.subspan(offset) };
  }

private:
  std::array<char, SIZE> _characters{};
};

// One word of a format's argument list, as the game's printf reads it: a number, or the text a %s prints (a near
// address, or characters).
class FormatArgument {
public:
  template <typename T>
    requires std::integral<T> || std::is_enum_v<T>
  FormatArgument(T word) noexcept : _word{ static_cast<std::uint16_t>(word) } { } // NOLINT(misc-explicit-constructor): a word
  FormatArgument(Text text) noexcept : _text{ text } { }                          // NOLINT(misc-explicit-constructor): a text
  FormatArgument(TextTarget target) noexcept : _text{ target.Written() } { }      // NOLINT(misc-explicit-constructor): a text written
  template <std::size_t SIZE>
  FormatArgument(TextBuffer<SIZE> const& buffer) noexcept : _text{ buffer } { } // NOLINT(misc-explicit-constructor): a buffer

  // The word: a number, or the near address of a near text.
  [[nodiscard]] auto Word() const -> std::uint16_t {
    return _text ? _text->Near() : _word;
  }
  // The text a %s prints.
  [[nodiscard]] auto AsText() const noexcept -> Text {
    return _text ? *_text : Text{ _word };
  }

private:
  std::uint16_t       _word{ 0 };
  std::optional<Text> _text;
};

// The game's printf formatting of argument words: %[-][+][0][width][.precision][l](d|u|x|o|s|c), case insensitive,
// digits upper case; %s takes a text, %ld two words (low, high); any other character after % is copied as it is. A word
// the format asks for beyond the list reads as 0.
[[nodiscard]] auto FormatText(DataSegment const& data, std::string_view format, std::span<FormatArgument const> arguments) -> std::string;

// Writes the text with its terminator to the target.
inline auto StoreText(DataSegment& data, TextTarget target, std::string_view text) -> void {
  auto const characters{ target.Characters(data) };
  Expects(text.size() < characters.size(), "a text fits its buffer with its terminator");
  *std::ranges::copy(text, characters.begin()).out = '\0';
}

} // namespace opeu::game::detail::text

namespace opeu::game {
using detail::text::CString;
using detail::text::FormatArgument;
using detail::text::FormatText;
using detail::text::SegmentCharacters;
using detail::text::StoreText;
using detail::text::Text;
using detail::text::TextBuffer;
using detail::text::TextTarget;
} // namespace opeu::game

// The game's routines are global functions until #17 moves them into their namespaces; they name these unqualified.
using opeu::game::FormatArgument;
using opeu::game::Text;
using opeu::game::TextBuffer;
using opeu::game::TextTarget;
