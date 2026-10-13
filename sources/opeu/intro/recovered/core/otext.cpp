/* OPEN 02a0:0002..07c1 (image 0x2a02..0x31c2): keyboard read, the formatter
 * and the Japanese text console of the opening program. It is the OPEN build
 * of MAIN's TEXTOUT (0480): input_flush,
 * format_decimal, put_unsigned, put_long, text_clamp, army_position,
 * text_set_x/y, text_color, text_paper_set, text_control, cursor_print,
 * text_put_range and text_erase_char keep MAIN's C text. The two bytes at
 * 02a0:0000 end the preceding assembly segment (028d).
 *
 * Differences from MAIN: key_read returns the character without the keypad
 * mapping; opeu_format passes every output character to a far callback
 * (opeu_text_put for cursor_print, opeu_format_store for request_format)
 * instead of buffering 200 bytes, and ignores %l numbers (put_long is
 * linked but no longer called); there is no word-wrapping text, scroll or
 * window state; lines are 16 pixels, 32 in large mode; glyph drawing is an
 * empty routine; Shift-JIS lead bytes (80-9f, e0-fc) are buffered and the
 * pair is converted to JIS X 0208 (opeu_open_sjis_to_jis); ESC Y n selects
 * large mode (n == '1'); ESC S, ESC E and ESC W call the empty stubs
 * 062d:0416/0418/041a (they take the place of MAIN's 0541:011e, sound reset
 * and pending finish).
 *
 * DGROUP holds the console state (DS:015a..0177) and the erase string
 * (DS:0142).
 *
 * Escape sequences (ESC = 1bh) understood by opeu_text_put:
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

#include <algorithm>

/* DS is explicit and near pointers are 16-bit DS offsets. The original's
 * automatic conversion buffer lives on the DGROUP stack (SS = DS); here it
 * is reserved on the opening's stack so that callees receive DS offsets too.
 * Variadic entries receive the DS offset of their argument words, which is
 * the original stack layout after the return address. */
namespace opeu::intro::detail::otext {
namespace {
using enum TextEventId;
using TextOut = void (*)(Open& open, TextHost* host, uint16_t c);

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

auto Event(Open& open, TextHost* host, TextEventId id) -> uint16_t {
  return host->event(open, id, nullptr, 0);
}

template <class... Words>
auto Event(Open& open, TextHost* host, TextEventId id, Words... words) -> uint16_t {
  auto const     event{ host->event };
  uint16_t const args[]{ static_cast<uint16_t>(words)... };
  return event(open, id, args, sizeof...(Words));
}
}

auto opeu_key_read(Open& open, TextHost* host) -> uint8_t;
auto opeu_format_decimal(Open& open, TextHost* host, uint16_t* p) -> int16_t;
auto opeu_put_unsigned(Open& open, TextHost* host, uint16_t* p, uint16_t v, uint16_t base) -> int16_t;
auto opeu_put_long(Open& open, TextHost* host, uint16_t* p, int32_t v, int16_t base) -> int16_t;
void opeu_format(Open& open, TextHost* host, uint16_t args, TextOut out);
auto opeu_open_sjis_to_jis(Open& open, TextHost* host, uint16_t code) -> uint16_t;
void opeu_text_clamp(Open& open, TextHost* host);
void opeu_army_position(Open& open, TextHost* host, int16_t x, int16_t y);
void opeu_text_set_x(Open& open, TextHost* host, int16_t x);
void opeu_text_set_y(Open& open, TextHost* host, int16_t y);
void opeu_text_color(Open& open, TextHost* host, uint16_t color);
void opeu_text_paper_set(Open& open, TextHost* host, uint8_t paper);
void opeu_text_glyph(Open& open, TextHost* host, int16_t x, int16_t y, int16_t attribute, uint16_t code);
void opeu_text_newline(Open& open, TextHost* host);
void opeu_text_control(Open& open, TextHost* host, uint8_t c);
void opeu_format_store(Open& open, TextHost* host, char c);
void opeu_request_format(Open& open, TextHost* host, uint16_t frame);
void opeu_text_put_range(Open& open, TextHost* host, uint16_t s, uint8_t start, uint8_t count);
void opeu_text_erase_char(Open& open, TextHost* host);

/* The far callbacks the original passes to opeu_format (02a0:04bc and
 * 02a0:0712) receive the character in AX. */
static void host_put_out(Open& open, TextHost* host, uint16_t c) {
  opeu_text_put(open, host, static_cast<uint8_t>(c));
}

static void host_store_out(Open& open, TextHost* host, uint16_t c) {
  opeu_format_store(open, host, static_cast<char>(c));
}

/* 0002: wait for a key (INT 16h) and return its character. */
auto opeu_key_read(Open& open, TextHost* host) -> uint8_t {
  uint16_t r = 0;
  r          = Event(open, host, KEY_READ);
  return static_cast<uint8_t>(r);
}

/* 0022 */
void opeu_input_flush(Open& open, TextHost* host) {
  while (Event(open, host, KEY_WAITING) != 0u) opeu_key_read(open, host);
}

/* 0032: unsigned decimal at *p; *p is left at the first non-digit. */
auto opeu_format_decimal(Open& open, [[maybe_unused]] TextHost* host, uint16_t* p) -> int16_t {
  uint16_t a = 0;
  uint16_t b = 0;
  a          = 0;
  while ((b = Byte(open, *p) - '0') < 10) {
    a = static_cast<uint16_t>(b + (a * 10));
    ++*p;
  }
  return static_cast<int16_t>(a);
}

/* 006a/00d0: digits of v in base, uppercase; return the digit count. */
auto opeu_put_unsigned(Open& open, TextHost* host, uint16_t* p, uint16_t v, uint16_t base) -> int16_t {
  int16_t a = 0;
  if (v < base) {
    Byte(open, *p) = static_cast<uint8_t>(v + (v < 10 ? '0' : '7'));
    ++*p;
    return 1;
  }
  a = opeu_put_unsigned(open, host, p, v / base, base);
  opeu_put_unsigned(open, host, p, v % base, base);
  return static_cast<int16_t>(a + 1);
}

auto opeu_put_long(Open& open, TextHost* host, uint16_t* p, int32_t v, int16_t base) -> int16_t {
  int16_t a = 0;
  if (v < base) {
    Byte(open, *p) = static_cast<uint8_t>(static_cast<char>(v) + (v < 10 ? '0' : '7'));
    ++*p;
    return 1;
  }
  a = opeu_put_long(open, host, p, v / base, base);
  opeu_put_long(open, host, p, v % base, base);
  return static_cast<int16_t>(a + 1);
}

/* 0168: args addresses the format pointer followed by its arguments; every
 * formatted character is passed to out. */
// The opening's recovered control flow, one branch
// per format code; it goes when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void opeu_format(Open& open, TextHost* host, uint16_t args, TextOut out) {
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
        e = opeu_format_decimal(open, host, &j);
      else
        e = 0;
      c = Byte(open, j++);
      if (c == '.') {
        d = opeu_format_decimal(open, host, &j);
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
        if (f == 0) e = static_cast<int16_t>(e - opeu_put_unsigned(open, host, &h, static_cast<uint16_t>(NextInt(open, p)), static_cast<uint16_t>(i)));
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
          while (e-- > 0) out(open, host, static_cast<uint16_t>(a));
        while (Byte(open, h) != 0u) {
          if ((m == 0) || d > 0) {
            out(open, host, static_cast<uint16_t>(Byte(open, h)));
            d--;
          }
          h++;
        }
        if (b != 0)
          while (e-- > 0) out(open, host, static_cast<uint16_t>(' '));
        continue;
      case 0: goto done;
      }
    }
    out(open, host, static_cast<uint16_t>(c));
  }
done:;
}

/* 038e: Shift-JIS to JIS X 0208; 2228h for a code without a lead byte. */
auto opeu_open_sjis_to_jis([[maybe_unused]] Open& open, [[maybe_unused]] TextHost* host, uint16_t code) -> uint16_t {
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

void opeu_text_clamp(Open& open, [[maybe_unused]] TextHost* host) {
  Data(open).text_x = std::clamp<int16_t>(Data(open).text_x, 0, 79);
  Data(open).text_y = std::clamp<int16_t>(Data(open).text_y, 0, 399);
}

/* 0426: a negative coordinate keeps the current one. */
void opeu_army_position(Open& open, TextHost* host, int16_t x, int16_t y) {
  if (x >= 0) Data(open).text_x = x;
  if (y >= 0) Data(open).text_y = y;
  opeu_text_clamp(open, host);
}

void opeu_text_set_x(Open& open, TextHost* host, int16_t x) {
  Data(open).text_x = x;
  opeu_text_clamp(open, host);
}

void opeu_text_set_y(Open& open, TextHost* host, int16_t y) {
  Data(open).text_y = y;
  opeu_text_clamp(open, host);
}

void opeu_text_color(Open& open, [[maybe_unused]] TextHost* host, uint16_t color) {
  if (color <= 7) Data(open).text_ink = static_cast<uint8_t>(color);
}

void opeu_text_paper_set(Open& open, [[maybe_unused]] TextHost* host, uint8_t paper) {
  Data(open).text_paper = paper;
}

/* 045a: the glyph output of this build draws nothing. */
void opeu_text_glyph([[maybe_unused]] Open& open, [[maybe_unused]] TextHost* host, [[maybe_unused]] int16_t x, [[maybe_unused]] int16_t y, [[maybe_unused]] int16_t attribute,
                     [[maybe_unused]] uint16_t code) { }

/* 045e: lines are 16 pixels, 32 in large mode. */
void opeu_text_newline(Open& open, [[maybe_unused]] TextHost* host) {
  int16_t a         = 0;
  Data(open).text_x = Data(open).text_left;
  a                 = static_cast<int16_t>(Data(open).text_y + (Data(open).text_large != 0 ? 32 : 16));
  if (a <= 399) Data(open).text_y = a;
}

void opeu_text_control(Open& open, TextHost* host, uint8_t c) {
  switch (c) {
  case 10: opeu_text_newline(open, host); break;
  case 13: Data(open).text_x = Data(open).text_left; break;
  case 8:
    if (Data(open).text_x >= 1) Data(open).text_x--;
    break;
  case 7:  Event(open, host, STUB_B, 6); break;
  default: break;
  }
}

/* 04bc: console output of one byte; returns it. */
// The opening's recovered control flow, one branch
// per control code; it goes when the opening becomes a scene (M6).
// NOLINTNEXTLINE(readability-function-cognitive-complexity)
auto opeu_text_put(Open& open, TextHost* host, uint8_t c) -> uint8_t {
  uint16_t b = 0; /* character code */
  int16_t  d = 0; /* attribute */
  switch (Data(open).text_state) {
  case 1:
    switch (c) {
    case '=': Data(open).text_state = 2; break;
    case 'C': Data(open).text_state = 4; break;
    case 'R': Data(open).text_state = 5; break;
    case 'M': Data(open).text_state = 6; break;
    case 'B': Data(open).text_state = 7; break;
    case 'Y': Data(open).text_state = 8; break;
    case 'E': Event(open, host, STUB_B, 6); [[fallthrough]];
    case 'W': Data(open).text_state = 9; break;
    case 'T': Event(open, host, BOX, Data(open).text_x, Data(open).text_y, 80 - Data(open).text_x, (Data(open).text_large != 0) ? 32 : 16, Data(open).text_background); goto reset;
    case '*': Event(open, host, CLEAR); goto reset;
    case 'S': Event(open, host, STUB_A, 1); goto reset;
    default:  break;
    }
    break;
  case 2:
    opeu_text_set_y(open, host, static_cast<int16_t>((c - 0x20) << 5));
    Data(open).text_state = 3;
    break;
  case 3:
    opeu_text_set_x(open, host, static_cast<int16_t>(c - 0x20));
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
  case 9:
    Event(open, host, STUB_C, static_cast<uint8_t>(c - '0'));
    Data(open).text_state = 0;
    break;
  default:
    if (Data(open).text_lead != 0u) {
      b                    = opeu_open_sjis_to_jis(open, host, static_cast<uint16_t>(c | Data(open).text_lead << 8));
      Data(open).text_lead = 0;
    } else if ((c >= 0x80 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc)) {
      Data(open).text_lead = c;
      break;
    } else if (c == 0x1b) {
      Data(open).text_state = 1;
      break;
    } else if (c < 0x20) {
      opeu_text_control(open, host, c);
      break;
    } else {
      b = c;
    }
    if ((b >= 0x200 ? 78 : 79) < Data(open).text_x) opeu_text_newline(open, host);
    d = Data(open).text_background;
    if (d == 0) d = Data(open).text_paper;
    d = static_cast<int16_t>(d << 8 | Data(open).text_ink);
    if (Data(open).text_large != 0) d = static_cast<int16_t>(d | 0x8000);
    opeu_text_glyph(open, host, Data(open).text_x, Data(open).text_y, d, b);
    Data(open).text_x = static_cast<int16_t>(Data(open).text_x + (b >= 0x200 ? 2 : 1));
    break;
  }
  return c;
}

/* 0712: opeu_format output for opeu_request_format. */
void opeu_format_store(Open& open, [[maybe_unused]] TextHost* host, char c) {
  Byte(open, Data(open).format_output++) = static_cast<uint8_t>(c);
}

/* 071e */
void opeu_ui_text(Open& open, TextHost* host, uint16_t s) {
  while (Byte(open, s) != 0u) opeu_text_put(open, host, Byte(open, s++));
}

/* 073e: printf to the console. */
void opeu_cursor_print(Open& open, TextHost* host, uint16_t frame) {
  StackReservation const return_address{ open.State(), 4 };
  opeu_format(open, host, frame, host_put_out);
}

/* 0756: sprintf. */
void opeu_request_format(Open& open, TextHost* host, uint16_t frame) {
  StackReservation const return_address{ open.State(), 4 };
  Data(open).format_output = Word(open, frame);
  opeu_format(open, host, static_cast<uint16_t>(frame + 2), host_store_out);
  Byte(open, Data(open).format_output) = 0;
}

void opeu_text_put_range(Open& open, TextHost* host, uint16_t s, uint8_t start, uint8_t count) {
  uint16_t a = 0;
  a          = s + start;
  while ((count--) != 0u) opeu_text_put(open, host, Byte(open, a++));
}

/* 07a4: erase the previous character cell, keeping the paper. */
void opeu_text_erase_char(Open& open, TextHost* host) {
  uint8_t a = 0;
  a         = Data(open).text_paper;
  opeu_ui_text(open, host, Near(open, Data(open).text_erase));
  Data(open).text_paper = a;
}

}
