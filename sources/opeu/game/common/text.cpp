// The game's printf formatting (common/text.hpp).
#include "common/text.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace opeu::game::detail::text {
namespace {

// One directive's flags and sizes, as the formatter reads them before the conversion letter.
struct Spec {
  bool         left{ false };
  bool         plus{ false };
  bool         is_long{ false };
  bool         has_precision{ false };
  char         pad{ ' ' };
  std::int16_t width{ 0 };
  std::int16_t precision{ 6 };
};

[[nodiscard]] constexpr auto Upper(char c) noexcept -> char {
  return c >= 'a' && c <= 'z' ? static_cast<char>(c - 32) : c;
}

// The digits of an unsigned word in a base, upper case.
auto PutUnsigned(std::string& text, std::uint16_t value, std::uint16_t base) -> std::int16_t {
  if (value < base) {
    text += static_cast<char>(value < 10 ? value + '0' : value + '7');
    return 1;
  }
  auto const count{ PutUnsigned(text, value / base, base) };
  PutUnsigned(text, value % base, base);
  return static_cast<std::int16_t>(count + 1);
}

// The digits of a signed long in a base: a negative value prints as one character, as the game's formatter does.
auto PutLong(std::string& text, std::int32_t value, std::int16_t base) -> std::int16_t {
  if (value < base) {
    text += static_cast<char>(value < 10 ? static_cast<char>(value) + '0' : static_cast<char>(value) + '7');
    return 1;
  }
  auto const count{ PutLong(text, value / base, base) };
  PutLong(text, value % base, base);
  return static_cast<std::int16_t>(count + 1);
}

class Formatter {
public:
  Formatter(DataSegment const& data, std::string_view format, std::span<std::uint16_t const> words) noexcept : _data{ data }, _format{ format }, _words{ words } { }

  auto Run() -> std::string {
    for (char c; (c = Next()) != '\0';) {
      if (c != '%') {
        _output += c;
      } else if (auto const literal{ Convert() }; literal) {
        _output += *literal;
      } else if (_done) {
        break;
      }
    }
    return std::move(_output);
  }

private:
  [[nodiscard]] auto Peek() const noexcept -> char {
    return _at < _format.size() ? _format[_at] : '\0';
  }
  auto Next() noexcept -> char {
    auto const c{ Peek() };
    ++_at;
    return c;
  }
  // The unsigned decimal at the format position, in 16-bit arithmetic that wraps.
  auto Decimal() noexcept -> std::int16_t {
    std::uint16_t value{ 0 };
    for (std::uint16_t digit; (digit = static_cast<std::uint16_t>(static_cast<std::uint8_t>(Peek()) - '0')) < 10; ++_at) value = static_cast<std::uint16_t>(digit + value * 10);
    return static_cast<std::int16_t>(value);
  }
  auto NextWord() noexcept -> std::uint16_t {
    auto const index{ _argument++ };
    return index < _words.size() ? _words[index] : std::uint16_t{ 0 };
  }
  auto NextText() noexcept -> std::string_view {
    return CString(_data, NextWord());
  }
  auto NextLong() noexcept -> std::int32_t {
    auto const low{ NextWord() };
    return static_cast<std::int32_t>(static_cast<std::uint32_t>(NextWord()) << 16 | low);
  }

  auto Flags() noexcept -> struct Spec {
    Spec directive;
    if (Peek() == '-') {
      ++_at;
      directive.left = true;
    }
    if (Peek() == '+') {
      ++_at;
      directive.plus = true;
    }
    directive.pad   = Peek() == '0' ? '0' : ' ';
    directive.width = Peek() >= '0' && Peek() <= '9' ? Decimal() : std::int16_t{ 0 };
    return directive;
  }

  // Formats one directive after its '%'; the character to copy when the letter is no conversion, nothing otherwise
  // (and _done when the format ends inside the directive).
  auto Convert() -> std::optional<char> {
    auto directive{ Flags() };
    auto c{ Next() };
    if (c == '.') {
      directive.precision     = Decimal();
      directive.has_precision = true;
      c                       = Next();
    }
    if (Upper(c) == 'L') {
      directive.is_long = true;
      c                 = Next();
    }
    switch (Upper(c)) {
    case 'X':  Number(directive, 16, false); return std::nullopt;
    case 'D':  Number(directive, 10, true); return std::nullopt;
    case 'U':  Number(directive, 10, false); return std::nullopt;
    case 'O':  Number(directive, 8, false); return std::nullopt;
    case 'S':  String(directive); return std::nullopt;
    case 'C':  Character(directive); return std::nullopt;
    case '\0': _done = true; return std::nullopt;
    default:   return c;
    }
  }

  // A number: %d puts the sign before the digits and counts it against the width; the precision does not apply.
  void Number(Spec directive, std::int16_t base, bool is_signed) {
    std::string conversion;
    if (directive.is_long) {
      auto value{ NextLong() };
      if (is_signed) value = Sign(directive, conversion, value);
      directive.width -= PutLong(conversion, value, base);
    } else {
      auto value{ static_cast<std::int16_t>(NextWord()) };
      if (is_signed) value = static_cast<std::int16_t>(Sign(directive, conversion, value));
      directive.width -= PutUnsigned(conversion, static_cast<std::uint16_t>(value), static_cast<std::uint16_t>(base));
    }
    directive.has_precision = false;
    Pad(directive, conversion);
  }
  // The sign of a %d value: '-' and the value negated when negative, '+' with the + flag.
  static auto Sign(Spec& directive, std::string& conversion, std::int32_t value) -> std::int32_t {
    if (value < 0) {
      conversion += '-';
      directive.width--;
      return static_cast<std::int32_t>(0u - static_cast<std::uint32_t>(value));
    }
    if (directive.plus) {
      conversion += '+';
      directive.width--;
    }
    return value;
  }
  void String(Spec directive) {
    auto const text{ NextText() };
    directive.width = static_cast<std::int16_t>(directive.width - CString(text).size());
    Pad(directive, text);
  }
  void Character(Spec directive) {
    std::string const conversion(1, static_cast<char>(NextWord()));
    directive.width--;
    directive.has_precision = false;
    Pad(directive, conversion);
  }

  // The converted text (up to its terminator) padded to the width; a string's precision limits the characters copied.
  void Pad(Spec directive, std::string_view text) {
    if (!directive.left)
      while (directive.width-- > 0) _output += directive.pad;
    for (auto const c : CString(text)) {
      if (!directive.has_precision || directive.precision > 0) {
        _output += c;
        directive.precision--;
      }
    }
    if (directive.left)
      while (directive.width-- > 0) _output += ' ';
  }

  DataSegment const&             _data;
  std::string_view               _format;
  std::span<std::uint16_t const> _words;
  std::size_t                    _at{ 0 };
  std::size_t                    _argument{ 0 };
  std::string                    _output;
  bool                           _done{ false };
};

} // namespace

auto FormatText(DataSegment const& data, std::string_view format, std::span<std::uint16_t const> words) -> std::string {
  return Formatter{ data, format, words }.Run();
}

} // namespace opeu::game::detail::text
