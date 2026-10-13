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
void FormatArguments(Open& open, uint16_t args);
auto ShiftJisToJis(Open& open, uint16_t code) -> uint16_t;
void ClampCursor(Open& open);
void SetCursorColumn(Open& open, int16_t x);
void SetCursorRow(Open& open, int16_t y);
void NewLine(Open& open);
void ApplyControlCharacter(Open& open, uint8_t c);

// Waits for a key and returns its character.
auto ReadKey() -> uint8_t {
  uint16_t r = 0;
  r          = port_key_read();
  return static_cast<uint8_t>(r);
}

// Reads keys until none is waiting.
void FlushKeys() {
  while (opeu_turn_input_b() != 0u) ReadKey();
}

// The unsigned decimal at p; p is left at the first non-digit.
auto ParseDecimal(Open& open, uint16_t& p) -> int16_t {
  uint16_t a = 0;
  uint16_t b = 0;
  a          = 0;
  while ((b = Byte(open, p) - '0') < 10) {
    a = static_cast<uint16_t>(b + (a * 10));
    ++p;
  }
  return static_cast<int16_t>(a);
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

// printf: `args` addresses the format's near pointer followed by its arguments; every formatted character goes to
// the console.
// The opening's recovered control flow, one branch
// per format code; it goes when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void FormatArguments(Open& open, uint16_t args) {
  uint8_t                pad             = 0;
  int16_t                left_justify    = 0;
  uint8_t                code            = 0;
  int16_t                precision       = 0;
  int16_t                width           = 0;
  int16_t                long_argument   = 0;
  uint16_t               text            = 0;
  int16_t                base            = 0;
  uint16_t               format          = 0;
  int16_t                plus            = 0;
  uint16_t               conversion      = 0;
  int16_t                precision_given = 0;
  uint16_t               argument        = 0;
  StackReservation const reservation{ open.State(), 20 };
  conversion = reservation.Near();
  format     = Word(open, args);
  argument   = static_cast<uint16_t>(args + 2);
  while ((code = Byte(open, format++)) != 0) {
    if (code == '%') {
      text            = conversion;
      precision       = 6;
      plus            = 0;
      long_argument   = 0;
      precision_given = 0;
      left_justify    = 0;
      if (Byte(open, format) == '-') {
        format++;
        left_justify = 1;
      }
      if (Byte(open, format) == '+') {
        format++;
        plus = 1;
      }
      code = Byte(open, format);
      pad  = code == '0' ? '0' : ' ';
      if (code >= '0' && code <= '9')
        width = ParseDecimal(open, format);
      else
        width = 0;
      code = Byte(open, format++);
      if (code == '.') {
        precision       = ParseDecimal(open, format);
        precision_given = 1;
        code            = Byte(open, format++);
      }
      if (AsciiUpper(code) == 'L') {
        long_argument = 1;
        code          = Byte(open, format++);
      }
      switch (AsciiUpper(code)) {
      case 'X': base = 16; goto number;
      case 'D':
        if (long_argument != 0) {
          if (LongAt(open, argument) < 0) {
            SetLong(open, argument, NegateLong(LongAt(open, argument)));
            Byte(open, text) = '-';
            text++;
            width--;
          } else if (plus != 0) {
            Byte(open, text) = '+';
            text++;
            width--;
          }
        } else {
          if (IntAt(open, argument) < 0) {
            SetInt(open, argument, static_cast<int16_t>(-IntAt(open, argument)));
            Byte(open, text) = '-';
            text++;
            width--;
          } else if (plus != 0) {
            Byte(open, text) = '+';
            text++;
            width--;
          }
        }
        [[fallthrough]];
      case 'U': base = 10; goto number;
      case 'O':
        base = 8;
      number:
        if (long_argument == 0) width = static_cast<int16_t>(width - WriteDigits(open, text, static_cast<uint16_t>(NextInt(open, argument)), static_cast<uint16_t>(base)));
        goto term;
      case 'S':
        text  = NextWord(open, argument);
        width = static_cast<int16_t>(width - StringLength(open.Segment(), text));
        goto print;
      case 'C':
        Byte(open, text) = static_cast<uint8_t>(NextInt(open, argument));
        text++;
        width--;
      term:
        Byte(open, text) = 0;
        text             = conversion;
        precision_given  = 0;
      print:
        if (left_justify == 0)
          while (width-- > 0) PutCharacter(open, pad);
        while (Byte(open, text) != 0u) {
          if ((precision_given == 0) || precision > 0) {
            PutCharacter(open, Byte(open, text));
            precision--;
          }
          text++;
        }
        if (left_justify != 0)
          while (width-- > 0) PutCharacter(open, ' ');
        continue;
      case 0: goto done;
      }
    }
    PutCharacter(open, code);
  }
done:;
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

void ClampCursor(Open& open) {
  Data(open).text_x = std::clamp<int16_t>(Data(open).text_x, 0, 79);
  Data(open).text_y = std::clamp<int16_t>(Data(open).text_y, 0, 399);
}

void SetCursorColumn(Open& open, int16_t x) {
  Data(open).text_x = x;
  ClampCursor(open);
}

void SetCursorRow(Open& open, int16_t y) {
  Data(open).text_y = y;
  ClampCursor(open);
}

// Lines are 16 pixels, 32 in large mode.
void NewLine(Open& open) {
  int16_t a         = 0;
  Data(open).text_x = Data(open).text_left;
  a                 = static_cast<int16_t>(Data(open).text_y + (Data(open).text_large != 0 ? 32 : 16));
  if (a <= 399) Data(open).text_y = a;
}

void ApplyControlCharacter(Open& open, uint8_t c) {
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
      goto reset;
    case '*': opeu_vga_clear(); goto reset;
    case 'S': goto reset;
    default:  break;
    }
    break;
  case 2:
    SetCursorRow(open, static_cast<int16_t>((c - 0x20) << 5));
    Data(open).text_state = 3;
    break;
  case 3:
    SetCursorColumn(open, static_cast<int16_t>(c - 0x20));
  reset:
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
void PutString(Open& open, uint16_t s) {
  while (Byte(open, s) != 0u) PutCharacter(open, Byte(open, s++));
}

// printf to the console: the format's near pointer and its arguments at `frame`.
void PrintArguments(Open& open, uint16_t frame) {
  StackReservation const return_address{ open.State(), 4 };
  FormatArguments(open, frame);
}

void CursorPrint(Open& open, std::span<uint16_t const> words) {
  StackReservation const frame{ open.State(), static_cast<uint16_t>(2 * words.size()) };
  std::ranges::for_each(std::views::iota(0uz, words.size()), [&](std::size_t index) { SetWord(open, static_cast<uint16_t>(frame.Near() + (2 * index)), words[index]); });
  PrintArguments(open, frame.Near());
}

}
