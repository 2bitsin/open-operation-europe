// The text console: key reading, the printf-style formatter and the console's window, cursor, escape sequences and
// glyph output.
//
// Escape sequences (ESC = 1bh) understood by opeu_text_put:
//   ESC = row col  cursor y (row-20h)*32, x col-20h
//   ESC C n        ink n-'0'          ESC M n   paper n-'0'
//   ESC B n        background n-'0'   ESC R n   window left n-20h
//   ESC E n        sound reset, then pending finish (n-'0')
//   ESC W n        pending finish (n-'0')
//   ESC Y x        ignore x           ESC T     clear to end of line
//   ESC *          clear screen       ESC S     scroll (or wait for confirmation)
// opeu_ui_text buffers a word, wraps it at the window's right column and forwards these sequences; it drops DBCS
// lead bytes 81h..d8h together with the byte that follows the trail byte.
// Formatter directives: %[-][+][0][width][.precision][l](d|u|x|o|s|c), case insensitive; any other character after %
// is copied literally.
//
// Near pointers are DS offsets. The routines' buffers live on the DS stack below host->sp, so that the routines they
// call receive DS offsets too; the variadic routines receive the DS offset of their argument words.
#include "types.hpp"
#include "core/textout.hpp"
#include "core/memory.hpp"
#include "core/strings.hpp"
#include "common/events.hpp"
#include "common/host.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "io/keyboard.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"

#include <cstddef>
#include <utility>

using namespace opeu::game;

namespace {

// The services the console calls through host->event.
enum class TextService : uint16_t {
  KEY_READ =
      0, // wait for a key: scan code in the high byte, character in the low byte
  KEY_WAITING    = 1, // nonzero while a key is waiting
  WAIT_CONFIRM   = 2, // (value)
  BOX            = 3, // (x, y, width, height, colour)
  CLEAR          = 4, // clear the screen
  SOUND_RESET    = 5, // (6)
  PENDING_FINISH = 6, // (value)
  GLYPH_DRAW     = 7, // (routine offset, segment, x, y, 1, 16, attribute, bits)
};
using enum TextService;

// The glyph routines glyph_draw holds, by their code offset.
constexpr uint16_t GLYPH_TRANSPARENT{ 0x28a8 };
constexpr uint16_t GLYPH_OPAQUE{ 0x28c5 };

// The console's screen: 80 columns, 400 pixel rows, 16-pixel glyphs from code 20h.
constexpr int16_t  COLUMNS{ 80 };
constexpr int16_t  ROWS{ 400 };
constexpr uint16_t GLYPH_BYTES{ 16 };
constexpr uint8_t  FIRST_GLYPH{ 0x20 };
constexpr uint8_t  ESCAPE{ 0x1b };

// The states of the escape-sequence parser (text_state).
enum class EscapeState : int16_t {
  NONE       = 0,
  ESCAPE     = 1, // after ESC
  ROW        = 2, // ESC = row
  COLUMN     = 3, // ESC = row col
  INK        = 4, // ESC C
  LEFT       = 5, // ESC R
  PAPER      = 6, // ESC M
  BACKGROUND = 7, // ESC B
  IGNORE     = 8, // ESC Y
  FINISH     = 9, // ESC W, ESC E
};

// The byte, int and long at a near pointer.
auto At(uint8_t* ds, uint16_t at) -> uint8_t& {
  return ds[at];
}
auto Int(uint8_t* ds, uint16_t at) -> int16_t& {
  return *reinterpret_cast<int16_t*>(ds + at);
}
auto Long(uint8_t* ds, uint16_t at) -> int32_t& {
  return *reinterpret_cast<int32_t*>(ds + at);
}

// The next int or pointer of a variadic argument list at p.
auto NextInt(uint8_t* ds, uint16_t& p) -> int16_t {
  p += 2;
  return Int(ds, static_cast<uint16_t>(p - 2));
}
auto NextWord(uint8_t* ds, uint16_t& p) -> uint16_t {
  p += 2;
  return Word(ds, static_cast<uint16_t>(p - 2));
}

auto NegateLong(int32_t value) -> int32_t {
  return static_cast<int32_t>(0u - static_cast<uint32_t>(value));
}

// Reserves `size` bytes on the DS stack below host->sp; returns their near pointer.
auto Reserve(TextHost* host, uint16_t size) -> uint16_t {
  host->sp -= size;
  return host->sp;
}

auto State(DataSegment const& data) -> EscapeState {
  return static_cast<EscapeState>(data.text_state);
}
void SetState(DataSegment& data, EscapeState state) {
  data.text_state = std::to_underlying(state);
}

} // namespace

// Waits for a key; keypad scan codes become digits unless the character is CR or ESC.
uint8_t opeu_key_read(uint8_t ds[65536], TextHost* host) {
  auto const key{ Call(ds, host->event, KEY_READ) };
  auto const scan{ static_cast<uint8_t>(key >> 8) };
  auto       code{ static_cast<uint8_t>(key) };
  if (code == '\r' || code == ESCAPE)
    return code;
  switch (scan) {
    case 0x52: code = '0'; break;
    case 0x4f: code = '1'; break;
    case 0x50: code = '2'; break;
    case 0x51: code = '3'; break;
    case 0x4b: code = '4'; break;
    case 0x4c: code = '5'; break;
    case 0x4d: code = '6'; break;
    case 0x47: code = '7'; break;
    case 0x48: code = '8'; break;
    case 0x49: code = '9'; break;
  }
  return code;
}

// Reads every waiting key.
void opeu_input_flush(uint8_t ds[65536], TextHost* host) {
  while (Call(ds, host->event, KEY_WAITING))
    opeu_key_read(ds, host);
}

// The unsigned decimal at *p; *p is left at the first non-digit.
int16_t opeu_format_decimal(uint8_t ds[65536], [[maybe_unused]] TextHost* host,
                            uint16_t* p) {
  uint16_t value{ 0 };
  uint16_t digit;
  while ((digit = static_cast<uint16_t>(static_cast<uint8_t>(At(ds, *p)) -
                                        '0')) < 10) {
    value = static_cast<uint16_t>(digit + value * 10);
    ++*p;
  }
  return static_cast<int16_t>(value);
}

// The digits of v in base, uppercase, at *p; returns the digit count.
int16_t opeu_put_unsigned(uint8_t ds[65536], TextHost* host, uint16_t* p,
                          uint16_t v, uint16_t base) {
  if (v < base) {
    At(ds, *p) = static_cast<uint8_t>(v < 10 ? v + '0' : v + '7');
    ++*p;
    return 1;
  }
  auto const count{ opeu_put_unsigned(ds, host, p, v / base, base) };
  opeu_put_unsigned(ds, host, p, v % base, base);
  return static_cast<int16_t>(count + 1);
}

int16_t opeu_put_long(uint8_t ds[65536], TextHost* host, uint16_t* p, int32_t v,
                      int16_t base) {
  if (v < base) {
    At(ds, *p) = static_cast<uint8_t>(v < 10 ? static_cast<char>(v) + '0'
                                             : static_cast<char>(v) + '7');
    ++*p;
    return 1;
  }
  auto const count{ opeu_put_long(ds, host, p, v / base, base) };
  opeu_put_long(ds, host, p, v % base, base);
  return static_cast<int16_t>(count + 1);
}

// args addresses the format pointer followed by its arguments; the formatted text (up to 200 bytes) is passed to
// out.
void opeu_format(uint8_t ds[65536], TextHost* host, uint16_t args,
                 TextOut out) {
  uint8_t    pad;       // pad character
  int16_t    left;      // left-justify
  uint8_t    c;         // current character
  int16_t    precision; // precision
  int16_t    width;     // remaining width
  int16_t    is_long;   // long argument
  uint16_t   text;      // conversion text
  int16_t    base;      // base
  int16_t    plus;      // explicit plus
  int16_t    has_precision;
  auto const output{ Reserve(host, 200) };
  auto const conversion{ Reserve(host, 50) };
  auto       format{ Word(ds, args) };
  auto       p{ static_cast<uint16_t>(args + 2) }; // next argument
  auto       n{ output };                          // output position
  while ((c = At(ds, format++)) != 0) {
    if (c == '%') {
      text          = conversion;
      precision     = 6;
      plus          = 0;
      is_long       = 0;
      has_precision = 0;
      left          = 0;
      if (At(ds, format) == '-') {
        format++;
        left = 1;
      }
      if (At(ds, format) == '+') {
        format++;
        plus = 1;
      }
      c   = At(ds, format);
      pad = c == '0' ? '0' : ' ';
      if (c >= '0' && c <= '9')
        width = opeu_format_decimal(ds, host, &format);
      else
        width = 0;
      c = At(ds, format++);
      if (c == '.') {
        precision     = opeu_format_decimal(ds, host, &format);
        has_precision = 1;
        c             = At(ds, format++);
      }
      if (opeu_ascii_upper(c) == 'L') {
        is_long = 1;
        c       = At(ds, format++);
      }
      switch (opeu_ascii_upper(c)) {
        case 'X': base = 16; goto number;
        case 'D':
          if (is_long) {
            if (Long(ds, p) < 0) {
              Long(ds, p)    = NegateLong(Long(ds, p));
              At(ds, text++) = '-';
              width--;
            } else if (plus) {
              At(ds, text++) = '+';
              width--;
            }
          } else {
            if (Int(ds, p) < 0) {
              Int(ds, p)     = static_cast<int16_t>(-Int(ds, p));
              At(ds, text++) = '-';
              width--;
            } else if (plus) {
              At(ds, text++) = '+';
              width--;
            }
          }
          [[fallthrough]];
        case 'U': base = 10; goto number;
        case 'O':
          base = 8;
number:
          if (is_long) {
            width -= opeu_put_long(ds, host, &text, Long(ds, p), base);
            p += 4;
          } else
            width -= opeu_put_unsigned(ds, host, &text,
                                       static_cast<uint16_t>(NextInt(ds, p)),
                                       static_cast<uint16_t>(base));
          goto term;
        case 'S':
          text = NextWord(ds, p);
          width -= opeu_string_length(ds, text);
          goto pad;
        case 'C':
          At(ds, text++) =
              static_cast<uint8_t>(static_cast<char>(NextInt(ds, p)));
          width--;
term:
          At(ds, text)  = 0;
          text          = conversion;
          has_precision = 0;
pad:
          if (!left)
            while (width-- > 0)
              At(ds, n++) = pad;
          while (At(ds, text)) {
            if (!has_precision || precision > 0) {
              At(ds, n++) = At(ds, text);
              precision--;
            }
            text++;
          }
          if (left)
            while (width-- > 0)
              At(ds, n++) = ' ';
          continue;
        case 0: goto done;
      }
    }
    At(ds, n++) = c;
  }
done:
  At(ds, n) = 0;
  out(ds, host, output);
  host->sp += 250;
}

// Word-wrapping text output with embedded escape sequences.
void opeu_ui_text(uint8_t ds[65536], TextHost* host, uint16_t s) {
  auto&      data{ Data(ds) };
  auto const word{ Reserve(host, 32) }; // the pending word
  uint8_t    b;                         // current byte
  uint16_t   skip_spaces{ 0 };          // skip leading spaces after a wrap
  uint16_t   end{ word };               // word end
  uint16_t   next{ word };              // next byte to output
  uint16_t   printable{ 0 };            // printable width of the word
  // Whether the pending word does not fit before the window's right column.
  auto const overflows{ [&] {
    return static_cast<uint16_t>(data.text_x + printable) >
           static_cast<uint16_t>(data.text_right);
  } };
  auto const flush{ [&] {
    while (next != end) {
      opeu_text_put(ds, host, At(ds, next));
      next++;
    }
  } };
  b = At(ds, s);
  while (b) {
    if (skip_spaces) {
      skip_spaces = 0;
      while (b == ' ')
        b = At(ds, ++s);
    }
    if (b >= 0x81 && b <= 0xd8) {
      if ((b = At(ds, ++s)) != 0)
        s++;
    } else if (b == ESCAPE) {
      if ((b = At(ds, ++s)) == 0)
        break;
      switch (b) {
        case 'E':
        case '*':
        case 'S':
        case 'T':
          At(ds, end++) = ESCAPE;
          At(ds, end++) = b;
          break;
        case 'Y':
        case 'B':
        case 'C':
        case 'M':
        case 'R':
        case 'W':
          if (At(ds, s + 1) == 0)
            break;
          At(ds, end++) = ESCAPE;
          At(ds, end)   = b;
          b             = At(ds, ++s);
          end++;
          At(ds, end++) = b;
          break;
        case '=':
          if (At(ds, s + 1) == 0)
            break;
          if (At(ds, s + 2) == 0)
            break;
          At(ds, end++) = ESCAPE;
          At(ds, end++) = b;
          At(ds, end)   = At(ds, ++s);
          b             = At(ds, ++s);
          end++;
          At(ds, end++) = b;
          break;
      }
    } else if (b < ' ') {
      if (b == '\n') {
        if (overflows() && !opeu_text_scroll_check(ds, host))
          opeu_text_put(ds, host, '\n');
        flush();
        if (!opeu_text_scroll_check(ds, host))
          opeu_text_put(ds, host, '\n');
        next = end = word;
        printable  = 0;
      } else
        At(ds, end++) = b;
    } else if (b != ' ') {
      At(ds, end++) = b;
      printable++;
    } else {
      At(ds, end++) = b;
      if (overflows()) {
        if (!opeu_text_scroll_check(ds, host))
          opeu_text_put(ds, host, '\n');
        skip_spaces = 1;
      }
      flush();
      next = end = word;
      printable  = 0;
    }
    b = At(ds, ++s);
  }
  if (next != end) {
    if (overflows() && !opeu_text_scroll_check(ds, host))
      opeu_text_put(ds, host, '\n');
    flush();
  }
  host->sp += 32;
}

// Sets the scroll mode when it is 0 or 1; returns the current mode.
int16_t opeu_text_scroll_mode(uint8_t                    ds[65536],
                              [[maybe_unused]] TextHost* host, int16_t mode) {
  auto& data{ Data(ds) };
  if (mode == 1 || mode == 0)
    data.text_scroll = mode;
  return data.text_scroll;
}

void opeu_text_window(uint8_t ds[65536], [[maybe_unused]] TextHost* host,
                      int16_t left, int16_t right, int16_t top,
                      int16_t bottom) {
  auto& data{ Data(ds) };
  if (right > left && bottom > top) {
    data.text_left   = left;
    data.text_right  = right;
    data.text_top    = top;
    data.text_bottom = bottom;
  }
}

// In scroll mode, scrolls when the next line would pass the window's bottom; returns whether it did.
int16_t opeu_text_scroll_check(uint8_t ds[65536], TextHost* host) {
  auto const& data{ Data(ds) };
  if (data.text_scroll &&
      static_cast<uint16_t>(data.text_height + data.text_y) >
          static_cast<uint16_t>(data.text_bottom)) {
    opeu_text_scroll_up(ds, host);
    return 1;
  }
  return 0;
}

// Clears the window and homes the cursor to its top left.
void opeu_text_scroll_up(uint8_t ds[65536], TextHost* host) {
  auto const& data{ Data(ds) };
  Call(ds, host->event, WAIT_CONFIRM, 0);
  Call(ds, host->event, BOX, data.text_left, data.text_top,
       data.text_right - data.text_left, data.text_bottom - data.text_top + 2,
       data.text_background ? data.text_background : data.text_paper);
  opeu_army_position(ds, host, data.text_left, data.text_top);
}

int16_t opeu_text_get_left(uint8_t ds[65536], [[maybe_unused]] TextHost* host) {
  return Data(ds).text_left;
}
int16_t opeu_text_get_right(uint8_t                    ds[65536],
                            [[maybe_unused]] TextHost* host) {
  return Data(ds).text_right;
}
void opeu_text_set_left(uint8_t ds[65536], [[maybe_unused]] TextHost* host,
                        int16_t left) {
  Data(ds).text_left = left;
}
void opeu_text_set_right(uint8_t ds[65536], [[maybe_unused]] TextHost* host,
                         int16_t right) {
  Data(ds).text_right = right;
}

void opeu_text_columns(uint8_t ds[65536], [[maybe_unused]] TextHost* host,
                       int16_t left, int16_t right) {
  auto& data{ Data(ds) };
  if (right > left) {
    data.text_left  = left;
    data.text_right = right;
  }
}

// The window becomes the whole screen.
void opeu_text_window_reset(uint8_t                    ds[65536],
                            [[maybe_unused]] TextHost* host) {
  auto& data{ Data(ds) };
  data.text_right  = COLUMNS;
  data.text_left   = 0;
  data.text_top    = 0;
  data.text_bottom = ROWS;
}

int16_t opeu_text_set_height(uint8_t ds[65536], [[maybe_unused]] TextHost* host,
                             int16_t height) {
  if (height > 0) {
    Data(ds).text_height = height;
    return 1;
  }
  return 0;
}

void opeu_text_height_reset(uint8_t                    ds[65536],
                            [[maybe_unused]] TextHost* host) {
  Data(ds).text_height = GLYPH_BYTES;
}

// Keeps the cursor on the screen.
void opeu_text_clamp(uint8_t ds[65536], [[maybe_unused]] TextHost* host) {
  auto& data{ Data(ds) };
  data.text_x = data.text_x < 0             ? 0
                : data.text_x > COLUMNS - 1 ? COLUMNS - 1
                                            : data.text_x;
  data.text_y = data.text_y < 0          ? 0
                : data.text_y > ROWS - 1 ? ROWS - 1
                                         : data.text_y;
}

// Moves the cursor; a negative coordinate keeps the current one.
void opeu_army_position(uint8_t ds[65536], TextHost* host, int16_t x,
                        int16_t y) {
  auto& data{ Data(ds) };
  if (x >= 0)
    data.text_x = x;
  if (y >= 0)
    data.text_y = y;
  opeu_text_clamp(ds, host);
}

void opeu_text_set_x(uint8_t ds[65536], TextHost* host, int16_t x) {
  Data(ds).text_x = x;
  opeu_text_clamp(ds, host);
}

void opeu_text_set_y(uint8_t ds[65536], TextHost* host, int16_t y) {
  Data(ds).text_y = y;
  opeu_text_clamp(ds, host);
}

void opeu_text_color(uint8_t ds[65536], [[maybe_unused]] TextHost* host,
                     uint16_t color) {
  if (color <= 7)
    Data(ds).text_ink = static_cast<uint8_t>(color);
}

void opeu_text_paper_set(uint8_t ds[65536], [[maybe_unused]] TextHost* host,
                         uint8_t paper) {
  Data(ds).text_paper = paper;
}

int16_t opeu_text_get_x(uint8_t ds[65536], [[maybe_unused]] TextHost* host) {
  return Data(ds).text_x;
}
int16_t opeu_text_get_y(uint8_t ds[65536], [[maybe_unused]] TextHost* host) {
  return Data(ds).text_y;
}

// Saves the cursor, left column and colours.
void opeu_context_save(uint8_t ds[65536], [[maybe_unused]] TextHost* host) {
  auto& data{ Data(ds) };
  data.saved_x          = data.text_x;
  data.saved_y          = data.text_y;
  data.saved_left       = data.text_left;
  data.saved_paper      = data.text_paper;
  data.saved_ink        = data.text_ink;
  data.saved_background = data.text_background;
}

void opeu_context_restore(uint8_t ds[65536], [[maybe_unused]] TextHost* host) {
  auto& data{ Data(ds) };
  data.text_x          = data.saved_x;
  data.text_y          = data.saved_y;
  data.text_left       = data.saved_left;
  data.text_paper      = data.saved_paper;
  data.text_ink        = data.saved_ink;
  data.text_background = data.saved_background;
}

// Draws one 8x16 glyph of the codes 20h..7fh, opaque when a background colour is set.
void opeu_text_glyph(uint8_t ds[65536], TextHost* host, int16_t x, int16_t y,
                     int16_t attribute, uint16_t code) {
  auto const& data{ Data(ds) };
  auto const  bits{ Reserve(host, GLYPH_BYTES) };
  if (code >= FIRST_GLYPH && code <= 0x7f) {
    opeu_copy_far_to_near(
        ds, ds,
        static_cast<uint16_t>(offsetof(DataSegment, text_font) +
                              (code - FIRST_GLYPH) * GLYPH_BYTES),
        bits, GLYPH_BYTES);
    auto const& routine{ data.glyph_draw[data.text_background != 0] };
    Call(ds, host->event, GLYPH_DRAW, routine.offset, routine.segment, x, y, 1,
         16, attribute, bits);
  }
  host->sp += GLYPH_BYTES;
}

// Moves the cursor to the start of the next line, unless that passes the screen's bottom.
void opeu_text_newline(uint8_t ds[65536], [[maybe_unused]] TextHost* host) {
  auto& data{ Data(ds) };
  data.text_x = data.text_left;
  if (static_cast<uint16_t>(data.text_height + data.text_y) <= ROWS - 1)
    data.text_y += data.text_height;
}

// Line feed, carriage return, backspace and bell.
void opeu_text_control(uint8_t ds[65536], TextHost* host, uint8_t c) {
  auto& data{ Data(ds) };
  switch (c) {
    case '\n': opeu_text_newline(ds, host); break;
    case '\r': data.text_x = data.text_left; break;
    case '\b':
      if (data.text_x >= 1)
        data.text_x--;
      break;
    case '\a': Call(ds, host->event, SOUND_RESET, 6); break;
    default:   break;
  }
}

// Console output of one byte through the escape-sequence parser; returns the byte.
uint8_t opeu_text_put(uint8_t ds[65536], TextHost* host, uint8_t c) {
  auto&    data{ Data(ds) };
  int16_t  margin; // the right margin for this byte
  uint16_t code;   // the character code
  int16_t  attribute;
  switch (State(data)) {
    case EscapeState::ESCAPE:
      switch (c) {
        case '=': SetState(data, EscapeState::ROW); break;
        case 'C': SetState(data, EscapeState::INK); break;
        case 'R': SetState(data, EscapeState::LEFT); break;
        case 'M': SetState(data, EscapeState::PAPER); break;
        case 'B': SetState(data, EscapeState::BACKGROUND); break;
        case 'Y': SetState(data, EscapeState::IGNORE); break;
        case 'E': Call(ds, host->event, SOUND_RESET, 6); [[fallthrough]];
        case 'W': SetState(data, EscapeState::FINISH); break;
        case 'T':
          Call(ds, host->event, BOX, data.text_x, data.text_y,
               COLUMNS - data.text_x, data.text_height, data.text_background);
          goto reset;
        case '*': Call(ds, host->event, CLEAR); goto reset;
        case 'S':
          if (data.text_scroll)
            opeu_text_scroll_up(ds, host);
          else
            Call(ds, host->event, WAIT_CONFIRM, 1);
          goto reset;
        default: break;
      }
      break;
    case EscapeState::ROW:
      opeu_text_set_y(ds, host, static_cast<int16_t>((c - 0x20) << 5));
      SetState(data, EscapeState::COLUMN);
      break;
    case EscapeState::COLUMN:
      opeu_text_set_x(ds, host, static_cast<int16_t>(c - 0x20));
      [[fallthrough]];
    case EscapeState::IGNORE:
reset:
      SetState(data, EscapeState::NONE);
      break;
    case EscapeState::INK:
      data.text_ink = static_cast<uint8_t>(c - '0');
      SetState(data, EscapeState::NONE);
      break;
    case EscapeState::LEFT:
      data.text_left = static_cast<int16_t>(c - 0x20);
      SetState(data, EscapeState::NONE);
      break;
    case EscapeState::PAPER:
      data.text_paper = static_cast<uint8_t>(c - '0');
      SetState(data, EscapeState::NONE);
      break;
    case EscapeState::BACKGROUND:
      data.text_background = static_cast<uint8_t>(c - '0');
      SetState(data, EscapeState::NONE);
      break;
    case EscapeState::FINISH:
      Call(ds, host->event, PENDING_FINISH, static_cast<uint8_t>(c - '0'));
      SetState(data, EscapeState::NONE);
      break;
    default:
      if (data.text_lead) {
        code           = static_cast<uint16_t>(c | data.text_lead << 8);
        data.text_lead = 0;
      } else if (c >= 0x81 && c <= 0xb0) {
        data.text_lead = c;
        break;
      } else if (c == ESCAPE) {
        SetState(data, EscapeState::ESCAPE);
        break;
      } else if (c < 0x20) {
        opeu_text_control(ds, host, c);
        break;
      } else
        code = c;
      margin = c < 0x3a ? 79 : 77;
      if (margin < data.text_x)
        opeu_text_newline(ds, host);
      attribute = data.text_background;
      if (!attribute)
        attribute = data.text_paper;
      attribute = static_cast<int16_t>(attribute << 8 | data.text_ink);
      opeu_text_glyph(ds, host, data.text_x, data.text_y, attribute, code);
      data.text_x += code >= 0x200 ? 2 : 1;
      break;
  }
  return c;
}

// opeu_format's output for opeu_request_format: copies the text to format_output and advances it.
void opeu_format_store(uint8_t ds[65536], [[maybe_unused]] TextHost* host,
                       uint16_t s) {
  auto& data{ Data(ds) };
  while (At(ds, s)) {
    At(ds, data.format_output) = At(ds, s);
    s++;
    data.format_output++;
  }
  At(ds, data.format_output) = 0;
}

// sprintf: frame addresses the destination, the format and its arguments.
void opeu_request_format(uint8_t ds[65536], TextHost* host, uint16_t frame) {
  Data(ds).format_output = Word(ds, frame);
  opeu_format(ds, host, static_cast<uint16_t>(frame + 2), opeu_format_store);
}

// printf to the console.
void opeu_cursor_print(uint8_t ds[65536], TextHost* host, uint16_t frame) {
  opeu_format(ds, host, frame, opeu_ui_text);
}

void opeu_text_put_range(uint8_t ds[65536], TextHost* host, uint16_t s,
                         uint8_t start, uint8_t count) {
  auto at{ static_cast<uint16_t>(s + start) };
  while (count--)
    opeu_text_put(ds, host, At(ds, at++));
}

// Erases the previous character cell, keeping the paper.
void opeu_text_erase_char(uint8_t ds[65536], TextHost* host) {
  auto&      data{ Data(ds) };
  auto const paper{ data.text_paper };
  opeu_ui_text(ds, host, Near(data, data.text_erase));
  data.text_paper = paper;
}

// The console's services. While the console calls out, port_sp is lowered to its own sp, so that nested routines
// (which take the host again or reserve with port_stack_enter) allocate below its live buffers; both are restored
// when the service returns.
uint16_t svc_textout(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  uint16_t r             = 0;
  uint16_t saved_port_sp = port_sp, saved_host_sp = *port_textout_host_sp();
  if (*port_textout_host_sp() < port_sp)
    port_sp = *port_textout_host_sp();
  switch (static_cast<TextService>(id)) {
    case KEY_READ:    r = port_key_read(); break;
    case KEY_WAITING: r = opeu_turn_input_b(); break;
    case WAIT_CONFIRM:
      opeu_wait_confirm(ds, svc_keyboard, (int16_t)Arg(args, count, 0));
      break;
    case BOX:
      opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2),
                  Arg(args, count, 3), Arg(args, count, 4));
      break;
    case CLEAR:       opeu_vga_clear(); break;
    case SOUND_RESET: opeu_sound_reset(ds, svc_sound); break;
    case PENDING_FINISH:
      opeu_pending_finish(ds, svc_waitkey, (uint8_t)Arg(args, count, 0));
      break;
    case GLYPH_DRAW: {
      uint16_t       off = Arg(args, count, 0), seg = Arg(args, count, 1);
      const uint8_t* bits = PORT_DS(Arg(args, count, 7));
      if (seg == 0 && off == GLYPH_TRANSPARENT)
        opeu_draw_near(Arg(args, count, 2), Arg(args, count, 3),
                       Arg(args, count, 4), Arg(args, count, 5),
                       Arg(args, count, 6), bits);
      else if (seg == 0 && off == GLYPH_OPAQUE)
        opeu_code_0000_28c5(Arg(args, count, 2), Arg(args, count, 3),
                            Arg(args, count, 4), Arg(args, count, 5),
                            Arg(args, count, 6), bits);
      else
        port_fatal(
            "textout: glyph routine %04x:%04x is not a known VGALIB entry", seg,
            off);
      break;
    }
    default: port_unhandled("textout", id, count);
  }
  port_sp                 = saved_port_sp;
  *port_textout_host_sp() = saved_host_sp;
  return r;
}

void svc_textout_unused(uint8_t* a0, void* a1, uint16_t a2) {
  (void)a0;
  (void)a1;
  (void)a2;
  port_unhandled("textout (no service table)", (unsigned)(a2), 0);
}

auto port_routine_opeu_army_position(std::uint8_t*        ds,
                                     std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  opeu_army_position(ds, (TextHost*)port_textout_host(),
                     (short)Arg(args, count, 0), (short)Arg(args, count, 1));
  return 0;
}

auto port_routine_opeu_context_restore(std::uint8_t*        ds,
                                       std::uint16_t const* args,
                                       unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_context_restore(ds, (TextHost*)port_textout_host());
  return 0;
}

auto port_routine_opeu_context_save(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_context_save(ds, (TextHost*)port_textout_host());
  return 0;
}

// printf to the text cursor.
auto port_routine_opeu_cursor_print(std::uint8_t* ds, std::uint16_t const* args,
                                    unsigned count) -> std::uint32_t {
  btl_cursor_print(ds, args, count);
  return 0;
}

auto port_routine_opeu_input_flush(std::uint8_t* ds, std::uint16_t const* args,
                                   unsigned count) -> std::uint32_t {
  (void)args;
  (void)count;
  opeu_input_flush(ds, (TextHost*)port_textout_host());
  return 0;
}

// sprintf(destination, format, ...): every word as pushed.
auto port_routine_opeu_request_format(std::uint8_t*        ds,
                                      std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  btl_request_format(ds, args, count);
  return 0;
}

auto port_routine_opeu_text_color(std::uint8_t* ds, std::uint16_t const* args,
                                  unsigned count) -> std::uint32_t {
  opeu_text_color(ds, (TextHost*)port_textout_host(), Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_text_paper_set(std::uint8_t*        ds,
                                      std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  opeu_text_paper_set(ds, (TextHost*)port_textout_host(),
                      (uint8_t)Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_text_put_range(std::uint8_t*        ds,
                                      std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  opeu_text_put_range(ds, (TextHost*)port_textout_host(), Arg(args, count, 2),
                      (uint8_t)Arg(args, count, 0),
                      (uint8_t)Arg(args, count, 1));
  return 0;
}

auto port_routine_opeu_ui_text(std::uint8_t* ds, std::uint16_t const* args,
                               unsigned count) -> std::uint32_t {
  opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0));
  return 0;
}
