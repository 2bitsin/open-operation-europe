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
// Near pointers are DS offsets. The routines build their words and formatted texts as C++ strings; sprintf and printf
// receive their argument words as a span.
#include "core/textout.hpp"
#include "common/events.hpp"
#include "common/host.hpp"
#include "core/memory.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/text.hpp"
#include "io/keyboard.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <span>
#include <string>
#include <string_view>
#include <utility>

using namespace opeu::game;

namespace {

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

// The byte at a near pointer.
auto At(Game& game, uint16_t at) -> uint8_t& {
  return Byte(game, at);
}

// The console's word-wrapping printer: buffers a word, wraps it at the window's right column and forwards the escape
// sequences; it drops DBCS lead bytes 81h..d8h together with the byte that follows the trail byte.
class WordWrap {
public:
  WordWrap(Game& game, TextHost* host, std::string_view text) noexcept : _game{ game }, _host{ host }, _text{ text } { }

  void Print() {
    for (auto b{ Byte(_at) }; b; b = Byte(++_at)) {
      if (_skip_spaces) {
        _skip_spaces = false;
        while (b == ' ') b = Byte(++_at);
      }
      if (b >= 0x81 && b <= 0xd8) {
        if (Byte(++_at)) ++_at;
      } else if (b == ESCAPE) {
        if (!Escape()) break;
      } else if (b == '\n') {
        LineFeed();
      } else if (b != ' ') {
        _word += static_cast<char>(b);
        if (b >= ' ') _printable++;
      } else {
        Space();
      }
    }
    if (!_word.empty()) {
      WrapWhenOverflowing();
      Flush();
    }
  }

private:
  [[nodiscard]] auto Byte(std::size_t at) const noexcept -> uint8_t {
    return at < _text.size() ? static_cast<uint8_t>(_text[at]) : uint8_t{ 0 };
  }
  // Whether the pending word does not fit before the window's right column.
  [[nodiscard]] auto Overflows() const noexcept -> bool {
    auto const& data{ Data(_game) };
    return static_cast<uint16_t>(data.text_x + _printable) > static_cast<uint16_t>(data.text_right);
  }
  void NewLine() {
    if (!opeu_text_scroll_check(_game, _host)) opeu_text_put(_game, _host, '\n');
  }
  void WrapWhenOverflowing() {
    if (Overflows()) NewLine();
  }
  void Flush() {
    for (auto const c : _word) opeu_text_put(_game, _host, c);
    _word.clear();
    _printable = 0;
  }
  void LineFeed() {
    WrapWhenOverflowing();
    Flush();
    NewLine();
  }
  void Space() {
    _word += ' ';
    if (Overflows()) {
      NewLine();
      _skip_spaces = true;
    }
    Flush();
  }
  // Copies the escape sequence at the text position into the word; false when the text ends after the escape byte.
  auto Escape() -> bool {
    auto const b{ Byte(++_at) };
    if (!b) return false;
    switch (b) {
    case 'E':
    case '*':
    case 'S':
    case 'T': Append({ ESCAPE, b }); break;
    case 'Y':
    case 'B':
    case 'C':
    case 'M':
    case 'R':
    case 'W':
      if (Byte(_at + 1)) Append({ ESCAPE, b, Byte(++_at) });
      break;
    case '=':
      if (Byte(_at + 1) && Byte(_at + 2)) Append({ ESCAPE, b, Byte(_at + 1), Byte(_at + 2) });
      if (Byte(_at + 1) && Byte(_at + 2)) _at += 2;
      break;
    default: break;
    }
    return true;
  }
  void Append(std::initializer_list<uint8_t> bytes) {
    std::ranges::for_each(bytes, [this](uint8_t c) { _word += static_cast<char>(c); });
  }

  Game&            _game;
  TextHost*        _host;
  std::string_view _text;
  std::size_t      _at{ 0 };
  std::string      _word;
  uint16_t         _printable{ 0 };
  bool             _skip_spaces{ false };
};

// The text of a format word followed by its argument words.
auto FormatWords(DataSegment const& data, std::span<uint16_t const> words) -> std::string {
  if (words.empty()) return {};
  return FormatText(data, CString(data, words.front()), words.subspan(1));
}

// Where a glyph goes: its cell and its colour attribute (paper << 8 | ink).
struct GlyphCell {
  int16_t x;
  int16_t y;
  int16_t attribute;
};

// Draws a glyph's 16 rows of bits with the drawing routine: transparent (only the ink) or opaque.
template <auto DRAW>
void DrawGlyphWith(GlyphCell const& cell, std::span<uint8_t const> bits) {
  DRAW(static_cast<uint16_t>(cell.x), static_cast<uint16_t>(cell.y), 1, GLYPH_BYTES, static_cast<uint16_t>(cell.attribute), bits.data());
}

// The glyph routines, by the enumerator glyph_draw holds.
constexpr auto GLYPH_ROUTINES = DispatchRows<GlyphDraw, void (*)(GlyphCell const&, std::span<uint8_t const>)>({
    { GlyphDraw::TRANSPARENT, DrawGlyphWith<opeu_draw_near>      },
    { GlyphDraw::OPAQUE,      DrawGlyphWith<opeu_code_0000_28c5> },
});

auto State(DataSegment const& data) -> EscapeState {
  return static_cast<EscapeState>(data.text_state);
}
void SetState(DataSegment& data, EscapeState state) {
  data.text_state = std::to_underlying(state);
}

} // namespace

// Waits for a key; keypad scan codes become digits unless the character is CR or ESC.
uint8_t opeu_key_read(Game& game, TextHost* host) {
  auto const key{ port_key_read() };
  auto const scan{ static_cast<uint8_t>(key >> 8) };
  auto       code{ static_cast<uint8_t>(key) };
  if (code == '\r' || code == ESCAPE) return code;
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
void opeu_input_flush(Game& game, TextHost* host) {
  while (opeu_turn_input_b()) opeu_key_read(game, host);
}

// Word-wrapping text output with embedded escape sequences.
void opeu_ui_text(Game& game, TextHost* host, uint16_t s) {
  WordWrap{ game, host, CString(Data(game), s) }.Print();
}

// Sets the scroll mode when it is 0 or 1; returns the current mode.
int16_t opeu_text_scroll_mode(Game& game, [[maybe_unused]] TextHost* host, int16_t mode) {
  auto& data{ Data(game) };
  if (mode == 1 || mode == 0) data.text_scroll = mode;
  return data.text_scroll;
}

void opeu_text_window(Game& game, [[maybe_unused]] TextHost* host, int16_t left, int16_t right, int16_t top, int16_t bottom) {
  auto& data{ Data(game) };
  if (right > left && bottom > top) {
    data.text_left   = left;
    data.text_right  = right;
    data.text_top    = top;
    data.text_bottom = bottom;
  }
}

// In scroll mode, scrolls when the next line would pass the window's bottom; returns whether it did.
int16_t opeu_text_scroll_check(Game& game, TextHost* host) {
  auto const& data{ Data(game) };
  if (data.text_scroll && static_cast<uint16_t>(data.text_height + data.text_y) > static_cast<uint16_t>(data.text_bottom)) {
    opeu_text_scroll_up(game, host);
    return 1;
  }
  return 0;
}

// Clears the window and homes the cursor to its top left.
void opeu_text_scroll_up(Game& game, TextHost* host) {
  auto const& data{ Data(game) };
  opeu_wait_confirm(game, static_cast<int16_t>(0));
  opeu_ui_box(data.text_left, data.text_top, data.text_right - data.text_left, data.text_bottom - data.text_top + 2, data.text_background ? data.text_background : data.text_paper);
  opeu_army_position(game, host, data.text_left, data.text_top);
}

void opeu_text_set_left(Game& game, [[maybe_unused]] TextHost* host, int16_t left) {
  Data(game).text_left = left;
}

// The window becomes the whole screen.
void opeu_text_window_reset(Game& game, [[maybe_unused]] TextHost* host) {
  auto& data{ Data(game) };
  data.text_right  = COLUMNS;
  data.text_left   = 0;
  data.text_top    = 0;
  data.text_bottom = ROWS;
}

// Keeps the cursor on the screen.
void opeu_text_clamp(Game& game, [[maybe_unused]] TextHost* host) {
  auto& data{ Data(game) };
  data.text_x = data.text_x < 0 ? 0 : data.text_x > COLUMNS - 1 ? COLUMNS - 1 : data.text_x;
  data.text_y = data.text_y < 0 ? 0 : data.text_y > ROWS - 1 ? ROWS - 1 : data.text_y;
}

// Moves the cursor; a negative coordinate keeps the current one.
void opeu_army_position(Game& game, TextHost* host, int16_t x, int16_t y) {
  auto& data{ Data(game) };
  if (x >= 0) data.text_x = x;
  if (y >= 0) data.text_y = y;
  opeu_text_clamp(game, host);
}

void opeu_text_set_x(Game& game, TextHost* host, int16_t x) {
  Data(game).text_x = x;
  opeu_text_clamp(game, host);
}

void opeu_text_set_y(Game& game, TextHost* host, int16_t y) {
  Data(game).text_y = y;
  opeu_text_clamp(game, host);
}

void opeu_text_color(Game& game, [[maybe_unused]] TextHost* host, uint16_t color) {
  if (color <= 7) Data(game).text_ink = static_cast<uint8_t>(color);
}

void opeu_text_paper_set(Game& game, [[maybe_unused]] TextHost* host, uint8_t paper) {
  Data(game).text_paper = paper;
}

// Saves the cursor, left column and colours.
void opeu_context_save(Game& game, [[maybe_unused]] TextHost* host) {
  auto& data{ Data(game) };
  data.saved_x          = data.text_x;
  data.saved_y          = data.text_y;
  data.saved_left       = data.text_left;
  data.saved_paper      = data.text_paper;
  data.saved_ink        = data.text_ink;
  data.saved_background = data.text_background;
}

void opeu_context_restore(Game& game, [[maybe_unused]] TextHost* host) {
  auto& data{ Data(game) };
  data.text_x          = data.saved_x;
  data.text_y          = data.saved_y;
  data.text_left       = data.saved_left;
  data.text_paper      = data.saved_paper;
  data.text_ink        = data.saved_ink;
  data.text_background = data.saved_background;
}

// Draws one 8x16 glyph of the codes 20h..7fh, opaque when a background colour is set.
void opeu_text_glyph(Game& game, [[maybe_unused]] TextHost* host, int16_t x, int16_t y, int16_t attribute, uint16_t code) {
  auto const& data{ Data(game) };
  if (code < FIRST_GLYPH || code > 0x7f) return;
  auto const bits{ std::span{ data.text_font }.subspan((code - FIRST_GLYPH) * GLYPH_BYTES, GLYPH_BYTES) };
  Dispatch(GLYPH_ROUTINES, data.glyph_draw[data.text_background != 0], GlyphCell{ x, y, attribute }, bits);
}

// Moves the cursor to the start of the next line, unless that passes the screen's bottom.
void opeu_text_newline(Game& game, [[maybe_unused]] TextHost* host) {
  auto& data{ Data(game) };
  data.text_x = data.text_left;
  if (static_cast<uint16_t>(data.text_height + data.text_y) <= ROWS - 1) data.text_y += data.text_height;
}

// Line feed, carriage return, backspace and bell.
void opeu_text_control(Game& game, TextHost* host, uint8_t c) {
  auto& data{ Data(game) };
  switch (c) {
  case '\n': opeu_text_newline(game, host); break;
  case '\r': data.text_x = data.text_left; break;
  case '\b':
    if (data.text_x >= 1) data.text_x--;
    break;
  case '\a': opeu_sound_reset(game); break;
  default:   break;
  }
}

// Console output of one byte through the escape-sequence parser; returns the byte.
uint8_t opeu_text_put(Game& game, TextHost* host, uint8_t c) {
  auto&    data{ Data(game) };
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
    case 'E': opeu_sound_reset(game); [[fallthrough]];
    case 'W': SetState(data, EscapeState::FINISH); break;
    case 'T': opeu_ui_box(data.text_x, data.text_y, COLUMNS - data.text_x, data.text_height, data.text_background); goto reset;
    case '*': opeu_vga_clear(); goto reset;
    case 'S':
      if (data.text_scroll)
        opeu_text_scroll_up(game, host);
      else
        opeu_wait_confirm(game, static_cast<int16_t>(1));
      goto reset;
    default: break;
    }
    break;
  case EscapeState::ROW:
    opeu_text_set_y(game, host, (c - 0x20) << 5);
    SetState(data, EscapeState::COLUMN);
    break;
  case EscapeState::COLUMN: opeu_text_set_x(game, host, c - 0x20); [[fallthrough]];
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
    opeu_pending_finish(game, c - '0');
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
      opeu_text_control(game, host, c);
      break;
    } else
      code = c;
    margin = c < 0x3a ? 79 : 77;
    if (margin < data.text_x) opeu_text_newline(game, host);
    attribute = data.text_background;
    if (!attribute) attribute = data.text_paper;
    attribute = static_cast<int16_t>(attribute << 8 | data.text_ink);
    opeu_text_glyph(game, host, data.text_x, data.text_y, attribute, code);
    data.text_x += code >= 0x200 ? 2 : 1;
    break;
  }
  return c;
}

// sprintf: the words are the destination, the format and its arguments. The text goes to the destination and
// format_output is left at its terminator.
void opeu_request_format(Game& game, std::span<uint16_t const> words) {
  auto&      data{ Data(game) };
  auto const destination{ words.empty() ? uint16_t{ 0 } : words.front() };
  auto const text{ FormatWords(data, words.subspan(std::min<std::size_t>(words.size(), 1))) };
  StoreText(data, destination, text);
  data.format_output = static_cast<uint16_t>(destination + text.size());
}

// printf to the console: the words are the format and its arguments.
void opeu_cursor_print(Game& game, std::span<uint16_t const> words) {
  WordWrap{ game, static_cast<TextHost*>(port_textout_host()), FormatWords(Data(game), words) }.Print();
}

void opeu_text_put_range(Game& game, TextHost* host, uint16_t s, uint8_t start, uint8_t count) {
  auto at{ static_cast<uint16_t>(s + start) };
  while (count--) opeu_text_put(game, host, At(game, at++));
}

// Erases the previous character cell, keeping the paper.
void opeu_text_erase_char(Game& game, TextHost* host) {
  auto&      data{ Data(game) };
  auto const paper{ data.text_paper };
  opeu_ui_text(game, host, Near(data, data.text_erase));
  data.text_paper = paper;
}
