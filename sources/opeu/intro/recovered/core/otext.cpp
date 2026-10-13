/* OPEN 02a0:0002..07c1 (image 0x2a02..0x31c2): keyboard read, the formatter
 * and the Japanese text console of the opening program. It is the OPEN build
 * of MAIN's TEXTOUT (0480): input_flush,
 * format_decimal, put_unsigned, put_long, text_clamp, army_position,
 * text_set_x/y, text_color, text_paper_set, text_control, cursor_print,
 * text_put_range and text_erase_char keep MAIN's C text. The two bytes at
 * 02a0:0000 end the preceding assembly segment (028d).
 *
 * Differences from MAIN: key_read returns the character without the keypad
 * mapping; FormatArguments passes every output character to a far callback
 * (PutCharacter for cursor_print, opeu_format_store for request_format)
 * instead of buffering 200 bytes, and ignores %l numbers (put_long is
 * linked but no longer called); there is no word-wrapping text, scroll or
 * window state; lines are 16 pixels, 32 in large mode; glyph drawing is an
 * empty routine; Shift-JIS lead bytes (80-9f, e0-fc) are buffered and the
 * pair is converted to JIS X 0208 (ShiftJisToJis); ESC Y n selects
 * large mode (n == '1'); ESC S, ESC E and ESC W call the empty stubs
 * 062d:0416/0418/041a (they take the place of MAIN's 0541:011e, sound reset
 * and pending finish).
 *
 * DGROUP holds the console state (DS:015a..0177) and the erase string
 * (DS:0142).
 *
 * Escape sequences (ESC = 1bh) understood by PutCharacter:
 *   ESC = row col  cursor y (row-20h)*32, x col-20h
 *   ESC C n  ink n-'0'     ESC M n  paper n-'0'    ESC B n  background n-'0'
 *   ESC R n  left n-20h    ESC Y n  large mode     ESC T    clear to line end
 *   ESC *    clear screen  ESC S    stub a(1)
 *   ESC E n  stub b(6), then stub c(n-'0')    ESC W n  stub c(n-'0')
 * Formatter directives: %[-][+][0][width][.precision][l](d|u|x|o|s|c), case
 * insensitive; any other character after % is copied literally.
 */
#include "recovered.hpp"
#include "segment-bytes.hpp"

#include "services.hpp"
#include "vgalib.hpp"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <span>

#include <algorithm>

/* DS is explicit and near pointers are 16-bit DS offsets. The original's
 * automatic conversion buffer lives on the DGROUP stack (SS = DS); here it
 * is reserved on the opening's stack so that callees receive DS offsets too.
 * Variadic entries receive the DS offset of their argument words, which is
 * the original stack layout after the return address. */
namespace opeu::intro::detail::otext {
namespace {
using TextOut = void (*)(Open& open, uint16_t c);

/* The int and long at a near pointer. */
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

/* The next int or pointer of a variadic argument list at p. */
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
void FormatArguments(Open& open, uint16_t args, TextOut out);
auto ShiftJisToJis(Open& open, uint16_t code) -> uint16_t;
void ClampCursor(Open& open);
void SetCursorColumn(Open& open, int16_t x);
void SetCursorRow(Open& open, int16_t y);
void NewLine(Open& open);
void ApplyControlCharacter(Open& open, uint8_t c);

/* The far callbacks the original passes to FormatArguments (02a0:04bc and
 * 02a0:0712) receive the character in AX. */
static void PrintCharacter(Open& open, uint16_t c) {
  PutCharacter(open, static_cast<uint8_t>(c));
}

/* 0002: wait for a key (INT 16h) and return its character. */
auto ReadKey() -> uint8_t {
  uint16_t r = 0;
  r          = port_key_read();
  return static_cast<uint8_t>(r);
}

/* 0022 */
void FlushKeys() {
  while (opeu_turn_input_b() != 0u) ReadKey();
}

/* 0032: unsigned decimal at *p; *p is left at the first non-digit. */
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

/* 006a/00d0: digits of v in base, uppercase; return the digit count. */
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

/* 0168: args addresses the format pointer followed by its arguments; every
 * formatted character is passed to out. */
// The opening's recovered control flow, one branch
// per format code; it goes when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void FormatArguments(Open& open, uint16_t args, TextOut out) {
  uint8_t                a = 0; /* pad character */
  int16_t                b = 0; /* left-justify */
  uint8_t                c = 0; /* current character */
  int16_t                d = 0; /* precision */
  int16_t                e = 0; /* remaining width */
  int16_t                f = 0; /* long argument */
  uint16_t               h = 0; /* conversion text */
  int16_t                i = 0; /* base */
  uint16_t               j = 0; /* format */
  int16_t                k = 0; /* explicit plus */
  uint16_t               l = 0; /* conversion buffer */
  int16_t                m = 0; /* precision given */
  uint16_t               p = 0; /* next argument */
  StackReservation const buffer{ open.State(), 20 };
  l = buffer.Near();
  j = Word(open, args);
  p = static_cast<uint16_t>(args + 2);
  while ((c = Byte(open, j++)) != 0) {
    if (c == '%') {
      h = l;
      d = 6;
      k = 0;
      f = 0;
      m = 0;
      b = 0;
      if (Byte(open, j) == '-') {
        j++;
        b = 1;
      }
      if (Byte(open, j) == '+') {
        j++;
        k = 1;
      }
      c = Byte(open, j);
      a = c == '0' ? '0' : ' ';
      if (c >= '0' && c <= '9')
        e = ParseDecimal(open, j);
      else
        e = 0;
      c = Byte(open, j++);
      if (c == '.') {
        d = ParseDecimal(open, j);
        m = 1;
        c = Byte(open, j++);
      }
      if (AsciiUpper(c) == 'L') {
        f = 1;
        c = Byte(open, j++);
      }
      switch (AsciiUpper(c)) {
      case 'X': i = 16; goto number;
      case 'D':
        if (f != 0) {
          if (LongAt(open, p) < 0) {
            SetLong(open, p, NegateLong(LongAt(open, p)));
            Byte(open, h) = '-';
            h++;
            e--;
          } else if (k != 0) {
            Byte(open, h) = '+';
            h++;
            e--;
          }
        } else {
          if (IntAt(open, p) < 0) {
            SetInt(open, p, static_cast<int16_t>(-IntAt(open, p)));
            Byte(open, h) = '-';
            h++;
            e--;
          } else if (k != 0) {
            Byte(open, h) = '+';
            h++;
            e--;
          }
        }
        [[fallthrough]];
      case 'U': i = 10; goto number;
      case 'O':
        i = 8;
      number:
        if (f == 0) e = static_cast<int16_t>(e - WriteDigits(open, h, static_cast<uint16_t>(NextInt(open, p)), static_cast<uint16_t>(i)));
        goto term;
      case 'S':
        h = NextWord(open, p);
        e = static_cast<int16_t>(e - StringLength(open.Segment(), h));
        goto pad;
      case 'C':
        Byte(open, h) = static_cast<uint8_t>(NextInt(open, p));
        h++;
        e--;
      term:
        Byte(open, h) = 0;
        h             = l;
        m             = 0;
      pad:
        if (b == 0)
          while (e-- > 0) out(open, static_cast<uint16_t>(a));
        while (Byte(open, h) != 0u) {
          if ((m == 0) || d > 0) {
            out(open, static_cast<uint16_t>(Byte(open, h)));
            d--;
          }
          h++;
        }
        if (b != 0)
          while (e-- > 0) out(open, static_cast<uint16_t>(' '));
        continue;
      case 0: goto done;
      }
    }
    out(open, static_cast<uint16_t>(c));
  }
done:;
}

/* 038e: Shift-JIS to JIS X 0208; 2228h for a code without a lead byte. */
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

/* 045e: lines are 16 pixels, 32 in large mode. */
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

/* 04bc: console output of one byte; returns it. */
// The opening's recovered control flow, one branch
// per control code; it goes when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto PutCharacter(Open& open, uint8_t c) -> uint8_t {
  uint16_t b = 0; /* character code */
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
    /* The opening's console draws no glyphs: a character only moves the cursor. */
    Data(open).text_x = static_cast<int16_t>(Data(open).text_x + (b >= 0x200 ? 2 : 1));
    break;
  }
  return c;
}

/* 071e */
void PutString(Open& open, uint16_t s) {
  while (Byte(open, s) != 0u) PutCharacter(open, Byte(open, s++));
}

/* 073e: printf to the console. */
void PrintArguments(Open& open, uint16_t frame) {
  StackReservation const return_address{ open.State(), 4 };
  FormatArguments(open, frame, PrintCharacter);
}

void CursorPrint(Open& open, std::span<uint16_t const> words) {
  StackReservation const frame{ open.State(), static_cast<uint16_t>(2 * words.size()) };
  std::ranges::for_each(std::views::iota(0uz, words.size()), [&](std::size_t index) { SetWord(open, static_cast<uint16_t>(frame.Near() + (2 * index)), words[index]); });
  PrintArguments(open, frame.Near());
}

}
