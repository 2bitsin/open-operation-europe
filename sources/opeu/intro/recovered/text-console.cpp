// The opening's text console: the key read, the printf formatter and the console that interprets escape sequences.
// It draws no glyphs (the opening prints only on an allocation or file error and to reset the console): a character
// moves the cursor, two columns for a Shift-JIS pair; lines are 16 pixels, 32 in large mode. The console state lives
// in DGROUP (text_x .. text_state).
//
// Escape sequences (ESC = 1bh) understood by PutCharacter:
//   ESC = row col  cursor y (row-20h)*32, x col-20h
//   ESC C n  ink n-'0'     ESC M n  paper n-'0'    ESC B n  background n-'0'
//   ESC R n  left n-20h    ESC Y n  large mode     ESC T    clear to line end
//   ESC *    clear screen  ESC S    nothing        ESC E n, ESC W n  n is skipped
// Formatter directives: %[-][+][0][width][.precision][l](d|u|x|o|s|c), case insensitive; any other character after %
// is copied literally; a %l number is negated in place but prints nothing.
//
// Near pointers are 16-bit DGROUP offsets. The formatter's conversion buffer is reserved on the opening's stack in
// DGROUP (SS = DS), so callees receive DGROUP offsets too; a printf receives the DGROUP offset of its argument words.
#include "recovered.hpp"
#include "repeat.hpp"
#include "segment-bytes.hpp"

#include "services.hpp"
#include "vgalib.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <span>

namespace opeu::intro::detail::text_console {
namespace {
// The int and long at a near pointer.
auto IntAt(Open const& open, uint16_t at) -> int16_t {
  return static_cast<int16_t>(Word(open, at));
}
auto SetInt(Open const& open, uint16_t at, int16_t value) -> void {
  SetWord(open, at, static_cast<uint16_t>(value));
}
auto LongAt(Open const& open, uint16_t at) -> int32_t {
  return static_cast<int32_t>(Word(open, at) | static_cast<uint32_t>(Word(open, static_cast<uint16_t>(at + 2))) << 16);
}
auto SetLong(Open const& open, uint16_t at, int32_t value) -> void {
  SetWord(open, at, static_cast<uint16_t>(value));
  SetWord(open, static_cast<uint16_t>(at + 2), static_cast<uint16_t>(static_cast<uint32_t>(value) >> 16));
}

// The next int or pointer of a variadic argument list at p.
auto NextInt(Open& open, uint16_t& p) -> int16_t {
  p += 2;
  return IntAt(open, static_cast<uint16_t>(p - 2));
}
auto NextWord(Open& open, uint16_t& p) -> uint16_t {
  p += 2;
  return Word(open, static_cast<uint16_t>(p - 2));
}

auto NegateLong(int32_t value) -> int32_t {
  return static_cast<int32_t>(0u - static_cast<uint32_t>(value));
}

}

auto ReadKey() -> uint8_t;
auto ParseDecimal(Open& open, uint16_t& p) -> int16_t;
auto WriteDigits(Open& open, uint16_t& p, uint16_t v, uint16_t base) -> int16_t;
auto FormatArguments(Open& open, uint16_t args) -> void;
auto ShiftJisToJis(Open& open, uint16_t code) -> uint16_t;
auto ClampCursor(Open& open) -> void;
auto SetCursorColumn(Open& open, int16_t x) -> void;
auto SetCursorRow(Open& open, int16_t y) -> void;
auto NewLine(Open& open) -> void;
auto ApplyControlCharacter(Open& open, uint8_t c) -> void;

// Waits for a key and returns its character.
auto ReadKey() -> uint8_t {
  uint16_t r = 0;
  r          = port_key_read();
  return static_cast<uint8_t>(r);
}

// Reads keys until none is waiting.
auto FlushKeys() -> void {
  RepeatWhile([] { return opeu_turn_input_b() != 0u; }, [] { ReadKey(); });
}

// The decimal number (no sign) at p; p is left at the first non-digit.
auto ParseDecimal(Open& open, uint16_t& p) -> int16_t {
  auto const digit{ [&] { return static_cast<uint16_t>(Byte(open, p) - '0'); } };
  uint16_t   value{ 0 };
  RepeatWhile([&] { return digit() < 10; },
              [&] {
                value = static_cast<uint16_t>(digit() + (value * 10));
                ++p;
              });
  return static_cast<int16_t>(value);
}

// The digits of v in base, upper case, written at p; the digit count.
auto WriteDigits(Open& open, uint16_t& p, uint16_t v, uint16_t base) -> int16_t {
  int16_t a = 0;
  if (v < base) {
    Byte(open, p) = static_cast<uint8_t>(v + (v < 10 ? '0' : '7'));
    ++p;
    return 1;
  }
  a = WriteDigits(open, p, v / base, base);
  WriteDigits(open, p, v % base, base);
  return static_cast<int16_t>(a + 1);
}

namespace {
// One printf directive after its '%': the flags, the width, the precision and the size, and its conversion code.
struct Directive {
  uint8_t code{ 0 };
  uint8_t pad{ ' ' };
  bool    left_justify{ false };
  bool    plus{ false };
  bool    long_argument{ false };
  bool    precision_given{ false };
  int16_t precision{ 6 };
  int16_t width{ 0 };
};

// printf over the opening's data segment: the format and its arguments at near pointers, the conversion buffer on the
// stack, every formatted character to the console.
class Formatter {
public:
  Formatter(Open& open, uint16_t args, uint16_t conversion) noexcept
      : _open{ open }, _format{ Word(open, args) }, _argument{ static_cast<uint16_t>(args + 2) }, _conversion{ conversion }, _text{ conversion } { }

  // Formats up to the format's end.
  auto Run() -> void {
    RepeatUntil([&] {
      auto const code{ NextFormatByte() };
      if (code == 0) return true;
      if (code != '%') {
        PutCharacter(_open, code);
        return false;
      }
      return !FormatDirective();
    });
  }

private:
  auto NextFormatByte() -> uint8_t {
    return Byte(_open, _format++);
  }

  // A '%' directive; false when the format ends inside it.
  auto FormatDirective() -> bool {
    ParseDirective();
    switch (AsciiUpper(_directive.code)) {
    case 'X': FormatNumber(16); break;
    case 'D':
      WriteSign();
      FormatNumber(10);
      break;
    case 'U': FormatNumber(10); break;
    case 'O': FormatNumber(8); break;
    case 'S': FormatString(); break;
    case 'C': FormatCharacter(); break;
    case 0:   return false;
    default:  PutCharacter(_open, _directive.code); break; // any other code is copied
    }
    return true;
  }

  // [-][+][0][width][.precision][l] and the conversion code.
  auto ParseDirective() -> void {
    _directive = Directive{};
    _text      = _conversion;
    if (Byte(_open, _format) == '-') {
      _format++;
      _directive.left_justify = true;
    }
    if (Byte(_open, _format) == '+') {
      _format++;
      _directive.plus = true;
    }
    auto const first{ Byte(_open, _format) };
    _directive.pad   = first == '0' ? '0' : ' ';
    _directive.width = (first >= '0' && first <= '9') ? ParseDecimal(_open, _format) : int16_t{ 0 };
    _directive.code  = NextFormatByte();
    if (_directive.code == '.') {
      _directive.precision       = ParseDecimal(_open, _format);
      _directive.precision_given = true;
      _directive.code            = NextFormatByte();
    }
    if (AsciiUpper(_directive.code) == 'L') {
      _directive.long_argument = true;
      _directive.code          = NextFormatByte();
    }
  }

  // A negative argument is made positive in place and its '-' written; a '+' when asked for.
  auto WriteSign() -> void {
    auto const negative{ _directive.long_argument ? LongAt(_open, _argument) < 0 : IntAt(_open, _argument) < 0 };
    if (negative) {
      if (_directive.long_argument) {
        SetLong(_open, _argument, NegateLong(LongAt(_open, _argument)));
      } else {
        SetInt(_open, _argument, static_cast<int16_t>(-IntAt(_open, _argument)));
      }
      Append('-');
    } else if (_directive.plus) {
      Append('+');
    }
  }

  // An int's digits; a %l number prints nothing more and its argument stays.
  auto FormatNumber(uint16_t base) -> void {
    if (!_directive.long_argument) _directive.width = static_cast<int16_t>(_directive.width - WriteDigits(_open, _text, static_cast<uint16_t>(NextInt(_open, _argument)), base));
    Terminate();
    Print();
  }

  // The string at the argument's near pointer, in place.
  auto FormatString() -> void {
    _text            = NextWord(_open, _argument);
    _directive.width = static_cast<int16_t>(_directive.width - StringLength(_open.Segment(), _text));
    Print();
  }

  auto FormatCharacter() -> void {
    Append(static_cast<uint8_t>(NextInt(_open, _argument)));
    Terminate();
    Print();
  }

  // One byte into the conversion buffer, taken from the width.
  auto Append(uint8_t character) -> void {
    Byte(_open, _text) = character;
    _text++;
    _directive.width--;
  }

  // Ends the conversion buffer's text and prints it from its start, whole.
  auto Terminate() -> void {
    Byte(_open, _text)         = 0;
    _text                      = _conversion;
    _directive.precision_given = false;
  }

  // The text, padded to the width on the side the directive asks; a given precision limits the characters.
  auto Print() -> void {
    if (!_directive.left_justify) PutRepeated(_directive.pad);
    RepeatWhile([&] { return Byte(_open, _text) != 0u; },
                [&] {
                  if (!_directive.precision_given || _directive.precision > 0) {
                    PutCharacter(_open, Byte(_open, _text));
                    _directive.precision--;
                  }
                  _text++;
                });
    if (_directive.left_justify) PutRepeated(' ');
  }

  // The padding the width leaves.
  auto PutRepeated(uint8_t character) -> void {
    std::ranges::for_each(std::views::iota(0, std::max<int>(_directive.width, 0)), [&](int) { PutCharacter(_open, character); });
  }

  Open&     _open;
  uint16_t  _format;
  uint16_t  _argument;
  uint16_t  _conversion;
  uint16_t  _text;
  Directive _directive;
};
} // namespace

// printf: `args` addresses the format's near pointer followed by its arguments; every formatted character goes to
// the console. The conversion buffer lies on the opening's stack.
auto FormatArguments(Open& open, uint16_t args) -> void {
  StackReservation const reservation{ open.State(), 20 };
  Formatter{ open, args, reservation.Near() }.Run();
}

// Shift-JIS to JIS X 0208; 2228h for a code without a lead byte.
auto ShiftJisToJis([[maybe_unused]] Open& open, uint16_t code) -> uint16_t {
  uint8_t a = 0;
  uint8_t b = 0;
  a         = static_cast<uint8_t>(code >> 8);
  if (a > 0x80 && a < 0xf0) {
    a = static_cast<uint8_t>((a - (a >= 0xa0 ? 0xb0 : 0x70)) << 1);
    b = static_cast<uint8_t>(code);
    if (b >= 0x80) b--;
    if (b >= 0x9e)
      b -= 0x5e;
    else
      a--;
    b -= 0x1f;
    return static_cast<uint16_t>(b | a << 8);
  }
  return 0x2228;
}

auto ClampCursor(Open& open) -> void {
  Data(open).text_x = std::clamp<int16_t>(Data(open).text_x, 0, 79);
  Data(open).text_y = std::clamp<int16_t>(Data(open).text_y, 0, 399);
}

auto SetCursorColumn(Open& open, int16_t x) -> void {
  Data(open).text_x = x;
  ClampCursor(open);
}

auto SetCursorRow(Open& open, int16_t y) -> void {
  Data(open).text_y = y;
  ClampCursor(open);
}

// Lines are 16 pixels, 32 in large mode.
auto NewLine(Open& open) -> void {
  int16_t a         = 0;
  Data(open).text_x = Data(open).text_left;
  a                 = static_cast<int16_t>(Data(open).text_y + (Data(open).text_large != 0 ? 32 : 16));
  if (a <= 399) Data(open).text_y = a;
}

auto ApplyControlCharacter(Open& open, uint8_t c) -> void {
  switch (c) {
  case 10: NewLine(open); break;
  case 13: Data(open).text_x = Data(open).text_left; break;
  case 8:
    if (Data(open).text_x >= 1) Data(open).text_x--;
    break;
  default: break;
  }
}

// Console output of one byte; returns it.
// The opening's recovered control flow, one branch
// per control code; it goes when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto PutCharacter(Open& open, uint8_t c) -> uint8_t {
  uint16_t b = 0; // character code
  switch (Data(open).text_state) {
  case 1:
    switch (c) {
    case '=': Data(open).text_state = 2; break;
    case 'C': Data(open).text_state = 4; break;
    case 'R': Data(open).text_state = 5; break;
    case 'M': Data(open).text_state = 6; break;
    case 'B': Data(open).text_state = 7; break;
    case 'Y': Data(open).text_state = 8; break;
    case 'E':
    case 'W': Data(open).text_state = 9; break;
    case 'T':
      opeu_ui_box(static_cast<uint16_t>(Data(open).text_x), static_cast<uint16_t>(Data(open).text_y), static_cast<uint16_t>(80 - Data(open).text_x), (Data(open).text_large != 0) ? 32 : 16,
                  Data(open).text_background);
      Data(open).text_state = 0;
      break;
    case '*':
      opeu_vga_clear();
      Data(open).text_state = 0;
      break;
    case 'S': Data(open).text_state = 0; break;
    default:  break;
    }
    break;
  case 2:
    SetCursorRow(open, static_cast<int16_t>((c - 0x20) << 5));
    Data(open).text_state = 3;
    break;
  case 3:
    SetCursorColumn(open, static_cast<int16_t>(c - 0x20));
    Data(open).text_state = 0;
    break;
  case 4:
    Data(open).text_ink   = c - '0';
    Data(open).text_state = 0;
    break;
  case 5:
    Data(open).text_left  = static_cast<int16_t>(c - 0x20);
    Data(open).text_state = 0;
    break;
  case 6:
    Data(open).text_paper = c - '0';
    Data(open).text_state = 0;
    break;
  case 7:
    Data(open).text_background = c - '0';
    Data(open).text_state      = 0;
    break;
  case 8:
    Data(open).text_large = c == '1' ? 1 : 0;
    Data(open).text_state = 0;
    break;
  case 9: Data(open).text_state = 0; break;
  default:
    if (Data(open).text_lead != 0u) {
      b                    = ShiftJisToJis(open, static_cast<uint16_t>(c | Data(open).text_lead << 8));
      Data(open).text_lead = 0;
    } else if ((c >= 0x80 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc)) {
      Data(open).text_lead = c;
      break;
    } else if (c == 0x1b) {
      Data(open).text_state = 1;
      break;
    } else if (c < 0x20) {
      ApplyControlCharacter(open, c);
      break;
    } else {
      b = c;
    }
    if ((b >= 0x200 ? 78 : 79) < Data(open).text_x) NewLine(open);
    // The opening's console draws no glyphs: a character only moves the cursor.
    Data(open).text_x = static_cast<int16_t>(Data(open).text_x + (b >= 0x200 ? 2 : 1));
    break;
  }
  return c;
}

// A string to the console.
auto PutString(Open& open, uint16_t s) -> void {
  RepeatWhile([&] { return Byte(open, s) != 0u; }, [&] { PutCharacter(open, Byte(open, s++)); });
}

// printf to the console: the format's near pointer and its arguments at `frame`.
auto PrintArguments(Open& open, uint16_t frame) -> void {
  StackReservation const return_address{ open.State(), 4 };
  FormatArguments(open, frame);
}

auto CursorPrint(Open& open, std::span<uint16_t const> words) -> void {
  StackReservation const frame{ open.State(), static_cast<uint16_t>(2 * words.size()) };
  std::ranges::for_each(std::views::iota(0uz, words.size()), [&](std::size_t index) { SetWord(open, static_cast<uint16_t>(frame.Near() + (2 * index)), words[index]); });
  PrintArguments(open, frame.Near());
}

}
