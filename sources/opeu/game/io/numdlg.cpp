// The five-digit numeric entry dialog with a slider bar. numdlg_x and numdlg_y place the dialog (an 8-pixel column
// and a pixel row), numdlg_digits holds its five decimal digits (the first may exceed 9). Its pictures are in the
// message picture segment; the graphics it covers are saved in numdlg_save_segment.
#include "io/numdlg.hpp"
#include "core/textout.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/host.hpp"
#include "common/memory.hpp"
#include "common/menu.hpp"
#include "core/numeric.hpp"
#include "core/uiprim.hpp"
#include "io/input-call.hpp"
#include "io/menudraw.hpp"
#include "io/mouse.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {

constexpr auto          DIGIT_COUNT{ 5 };
constexpr std::int32_t  TOP_PLACE{ 10000 };
constexpr std::uint16_t BAR_WIDTH{ 0x70 };
constexpr std::uint8_t  NOTHING{ 0xff };
constexpr std::uint8_t  TITLE_BAR{ 0 };
constexpr std::uint8_t  CANCEL_BUTTON{ 1 };
constexpr std::uint8_t  OK_BUTTON{ 2 };
constexpr std::uint8_t  FIRST_DIGIT{ 3 }; // hit codes 3..7 are the digits
constexpr std::uint8_t  LAST_DIGIT{ 7 };
constexpr std::uint16_t INVERT_MODE{ 0x107 };
constexpr std::uint16_t OUTLINE_COLOUR{ 0x17 };

// The button pictures in the message picture segment.
enum ButtonTile : std::uint16_t {
  OK_UP       = 0x1b00,
  OK_DOWN     = 0x1c00,
  CANCEL_UP   = 0x1d00,
  CANCEL_DOWN = 0x1e00,
};

auto Column(DataSegment const& data) -> std::uint16_t {
  return static_cast<std::uint16_t>(data.numdlg_x << 3);
}

// The UI outline reports each of its lines as its event 5; the bridge (passed as the outline's memory pointer)
// forwards them as UI_LINE.
struct OutlineBridge {
  HostService service;
};

auto OutlineLine(Game& game, std::uint8_t* memory, std::uint16_t id, std::uint16_t const* args, std::uint16_t count) -> std::uint32_t {
  constexpr std::uint16_t LINE_EVENT{ 5 };
  if (id != LINE_EVENT || count != 5) return 0;
  auto const* bridge{ reinterpret_cast<OutlineBridge const*>(memory) };
  Input(game, bridge->service, InputCall::UI_LINE, args[0], args[1], args[2], args[3], args[4]);
  return 0;
}

auto Outline(Game& game, HostService service, std::uint16_t x1, std::uint16_t y1, std::uint16_t x2, std::uint16_t y2) -> void {
  OutlineBridge bridge{ service };
  UiHost        host{ reinterpret_cast<std::uint8_t*>(&bridge), 0, 0, 0, OutlineLine };
  opeu_ui_outline(game, &host, x1, y1, x2, y2, OUTLINE_COLOUR);
}

// Clamps a dialog value to lo..hi, showing the clamped value.
auto Clamped(Game& game, HostService service, std::uint16_t lo, std::uint16_t hi, std::int32_t value) -> std::int32_t {
  if (value < lo) {
    value = lo;
    opeu_numdlg_show(game, service, value);
  } else if (value > hi) {
    value = hi;
    opeu_numdlg_show(game, service, value);
  }
  return value;
}
} // namespace

// Redraws button n's picture `tile`.
void opeu_numdlg_button(Game& game, HostService service, std::uint8_t n, std::uint16_t tile) {
  auto const& data{ Data(game) };
  opeu_mouse_hide(game, service);
  opeu_bitmap_restore(n * 5 + data.numdlg_x + 7, data.numdlg_y + 40, 4, 16, 0, port_far(data.message_picture_segment, tile));
  opeu_mouse_show(game, service);
}

// Button 1 (cancel) or 2 (OK) pressed.
void opeu_numdlg_press(Game& game, HostService service, std::uint8_t n) {
  opeu_numdlg_button(game, service, n - 1, n == CANCEL_BUTTON ? CANCEL_DOWN : OK_DOWN);
}

void opeu_numdlg_release(Game& game, HostService service, std::uint8_t n) {
  opeu_numdlg_button(game, service, n - 1, n == CANCEL_BUTTON ? CANCEL_UP : OK_UP);
}

// Saves the covered area, draws the dialog and both buttons.
void opeu_numdlg_open(Game& game, HostService service) {
  auto const& data{ Data(game) };
  opeu_bitmap_save(data.numdlg_x, data.numdlg_y, 0x12, 0x60, port_far(data.numdlg_save_segment, 0));
  opeu_bitmap_restore(data.numdlg_x, data.numdlg_y, 0x12, 0x60, 0, port_far(data.message_picture_segment, 0));
  opeu_numdlg_release(game, service, CANCEL_BUTTON);
  opeu_numdlg_release(game, service, OK_BUTTON);
}

void opeu_numdlg_close(Game& game) {
  auto const& data{ Data(game) };
  opeu_bitmap_restore(data.numdlg_x, data.numdlg_y, 0x12, 0x60, 0, port_far(data.numdlg_save_segment, 0));
}

void opeu_numdlg_digit(Game& game, std::uint8_t i, std::uint8_t digit) {
  auto const& data{ Data(game) };
  opeu_context_save(game, static_cast<TextHost*>(port_textout_host()));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(i * 3 + data.numdlg_x + 2), static_cast<std::int16_t>(data.numdlg_y + 0x40));
  opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(Near(data, data.numdlg_format)), static_cast<std::uint16_t>(digit), 0 });
  opeu_context_restore(game, static_cast<TextHost*>(port_textout_host()));
}

// Splits a value into the five digits (the first takes everything above 9999).
void opeu_numdlg_split(Game& game, std::int32_t value) {
  auto&        digits{ Data(game).numdlg_digits };
  std::int32_t place{ TOP_PLACE };
  for (auto c{ 0 }; c < DIGIT_COUNT; c++) {
    std::uint16_t const digit  = value / place;
    digits[c]                  = static_cast<std::uint8_t>(digit);
    value                     -= digit * place;
    place                     /= 10;
  }
}

auto opeu_numdlg_value(Game& game) -> std::int32_t {
  auto const&  digits{ Data(game).numdlg_digits };
  std::int32_t place{ TOP_PLACE };
  std::int32_t value{ 0 };
  for (auto c{ 0 }; c < DIGIT_COUNT; c++) {
    value += digits[c] * place;
    place /= 10;
  }
  return value;
}

void opeu_numdlg_show(Game& game, HostService service, std::int32_t value) {
  opeu_numdlg_split(game, value);
  opeu_mouse_hide(game, service);
  for (auto c{ 0 }; c < DIGIT_COUNT; c++) opeu_numdlg_digit(game, c, Data(game).numdlg_digits[c]);
  opeu_mouse_show(game, service);
}

// The slider: 0..BAR_WIDTH pixels filled for lo..hi.
void opeu_numdlg_bar(Game& game, HostService service, std::uint16_t lo, std::uint16_t hi, std::int32_t value) {
  auto const&         data{ Data(game) };
  std::uint16_t const x{ Column(data) };
  std::uint16_t const y{ data.numdlg_y };
  std::uint16_t const range = hi - lo;
  std::uint16_t       filled;
  if (value < lo)
    filled = 0;
  else if (value > hi || range == 0)
    filled = BAR_WIDTH;
  else
    filled = opeu_mul_div(static_cast<std::uint16_t>(value) - lo, BAR_WIDTH, range);
  opeu_mouse_hide(game, service);
  if (filled) opeu_ui_fill(x + 0x10, y + 0x14, x + filled + 0xf, y + 0x1b, 2);
  if (filled != BAR_WIDTH) opeu_ui_fill(x + filled + 0x10, y + 0x14, x + 0x7f, y + 0x1b, 0);
  opeu_mouse_show(game, service);
}

// What the mouse at (mx, my) is on: TITLE_BAR, CANCEL_BUTTON, OK_BUTTON, a digit (FIRST_DIGIT..LAST_DIGIT) or
// NOTHING.
auto opeu_numdlg_hit(Game& game, std::uint16_t mx, std::uint16_t my) -> std::uint8_t {
  auto const&         data{ Data(game) };
  std::uint8_t        hit{ NOTHING };
  std::uint16_t const x{ Column(data) };
  if (static_cast<std::uint16_t>(x + 0x10) > mx || static_cast<std::uint16_t>(data.numdlg_y + 0x20) > my) return NOTHING;
  std::uint16_t row = my - data.numdlg_y - 0x10;
  if (row % 24 < 16) {
    row                  /= 24;
    std::uint16_t column  = mx - x - 0x10;
    if (row == 1) {
      if (column % 40 < 32) {
        column /= 40;
        if (column > 2) column = NOTHING;
        hit = static_cast<std::uint8_t>(column);
      }
    } else if (row == 2) {
      if (column % 24 < 16) {
        row = column / 24 + FIRST_DIGIT;
        if (row > LAST_DIGIT) row = NOTHING;
        hit = static_cast<std::uint8_t>(row);
      }
    }
  }
  return hit;
}

// Inverts the title bar or a digit.
void opeu_numdlg_highlight(Game& game, HostService service, std::uint8_t n) {
  if (n == NOTHING || n == CANCEL_BUTTON || n == OK_BUTTON) return;
  std::uint16_t x, y, width;
  if (n == TITLE_BAR) {
    x     = 2;
    y     = 0x28;
    width = 4;
  } else {
    x     = (n - FIRST_DIGIT) * 3 + 2;
    y     = 0x40;
    width = 2;
  }
  auto const& data{ Data(game) };
  opeu_mouse_hide(game, service);
  opeu_ui_highlight(data.numdlg_x + x, data.numdlg_y + y, width, 0x10, INVERT_MODE);
  opeu_mouse_show(game, service);
}

// Moves the dialog with an outline while the left button is held.
void opeu_numdlg_drag(Game& game, HostService service) {
  auto& data{ Data(game) };
  opeu_mouse_hide(game, service);
  opeu_numdlg_close(game);
  opeu_mouse_show(game, service);
  auto const grab{ MousePosition(game, service) };
  // the grab point within the dialog; the mouse stays where the dialog stays on the screen
  std::uint16_t const grab_x = grab.x - Column(data);
  std::uint16_t const grab_y = grab.y - data.numdlg_y;
  opeu_mouse_window(game, service, grab_x, grab_y, grab_x + 0x1f0, grab_y + 0x130);
  std::uint16_t column{ 0xffff };
  std::uint16_t row{ 0xffff };
  do {
    auto [x, y]{ MousePosition(game, service) };
    if (x > 0x212) x = 0x1f4;
    if (y > 0x154) y = 0x154;
    x  -= grab_x;
    y  -= grab_y;
    x >>= 3;
    if (x != column || y != row) {
      if (column != 0xffff) {
        opeu_mouse_hide(game, service);
        Outline(game, service, column << 3, row, (column << 3) + 0x8f, row + 0x5f);
        opeu_mouse_show(game, service);
      }
      opeu_mouse_hide(game, service);
      Outline(game, service, x << 3, y, (x << 3) + 0x8f, y + 0x5f);
      opeu_mouse_show(game, service);
      column = x;
      row    = y;
    }
  } while (opeu_mouse_left(game, service));
  data.numdlg_x = column;
  data.numdlg_y = row;
  opeu_mouse_hide(game, service);
  Outline(game, service, column << 3, row, (column << 3) + 0x8f, row + 0x5f);
  opeu_numdlg_open(game, service);
  opeu_mouse_show(game, service);
  opeu_mouse_window(game, service, 0, 0, 0x27f, 0x18f);
}

void opeu_numdlg_refresh(Game& game, HostService service, std::uint8_t n) {
  opeu_mouse_hide(game, service);
  opeu_numdlg_digit(game, n, Data(game).numdlg_digits[n]);
  opeu_numdlg_highlight(game, service, n + FIRST_DIGIT);
  opeu_mouse_show(game, service);
}

// Digit n goes up, wrapping after 6 (the first) or 9; the value is clamped to lo..hi.
auto opeu_numdlg_increment(Game& game, HostService service, std::uint8_t n, std::uint16_t lo, std::uint16_t hi, std::int32_t value) -> std::int32_t {
  opeu_numdlg_split(game, value);
  auto&              digit{ Data(game).numdlg_digits[n] };
  std::uint8_t const top{ static_cast<std::uint8_t>(n == 0 ? 6 : 9) };
  if (++digit > top) digit = 0;
  value = Clamped(game, service, lo, hi, opeu_numdlg_value(game));
  opeu_numdlg_refresh(game, service, n);
  return value;
}

// Digit n goes down, wrapping from 0 to 6 (the first) or 9.
auto opeu_numdlg_decrement(Game& game, HostService service, std::uint8_t n, std::uint16_t lo, std::uint16_t hi, std::int32_t value) -> std::int32_t {
  opeu_numdlg_split(game, value);
  auto& digit{ Data(game).numdlg_digits[n] };
  if (digit == 0) digit = n == 0 ? 7 : 10;
  digit--;
  value = Clamped(game, service, lo, hi, opeu_numdlg_value(game));
  opeu_numdlg_refresh(game, service, n);
  return value;
}

// 0 inside the dialog's lower body x..x+8Fh, y+20h..y+5Fh.
auto opeu_numdlg_outside(Game& game, std::uint16_t mx, std::uint16_t my) -> std::uint16_t {
  auto const&         data{ Data(game) };
  std::uint16_t const x{ Column(data) };
  if (x <= mx && static_cast<std::uint16_t>(x + 0x90) > mx && static_cast<std::uint16_t>(data.numdlg_y + 0x20) <= my && static_cast<std::uint16_t>(data.numdlg_y + 0x60) > my) return 0;
  return 1;
}

// Runs the dialog until a value in lo..hi is accepted or (when allowed) the dialog is cancelled; returns the value or
// 0xffff.
auto opeu_numdlg_track(Game& game, HostService service, std::uint16_t lo, std::uint16_t hi, int cancel) -> std::uint16_t {
  auto const&  data{ Data(game) };
  std::uint8_t shown{ NOTHING };
  std::int32_t value{ lo };
  bool         running{ true };
  opeu_input_reset(game, service);
  opeu_mouse_show(game, service);
  opeu_numdlg_bar(game, service, lo, hi, lo);
  opeu_numdlg_show(game, service, lo);
  std::uint16_t left{ Column(data) };
  do {
    auto const mouse{ MousePosition(game, service) };
    if (static_cast<std::uint16_t>(data.numdlg_y + 0x20) > mouse.y) {
      // on the slider
      auto const previous{ value };
      auto const x{ mouse.x };
      if (static_cast<std::uint16_t>(left + 0x10) > x)
        value = lo;
      else if (static_cast<std::uint16_t>(left + 0x7f) <= x)
        value = hi;
      else
        value = static_cast<std::uint16_t>(opeu_mul_div(hi - lo + 1, x - left - 0xf, BAR_WIDTH) + lo);
      if (value != previous) {
        opeu_numdlg_bar(game, service, lo, hi, value);
        opeu_numdlg_show(game, service, value);
      }
    } else {
      auto const hit{ opeu_numdlg_hit(game, mouse.x, mouse.y) };
      if (hit != shown) {
        if (shown == CANCEL_BUTTON || shown == OK_BUTTON) opeu_numdlg_release(game, service, shown);
        if (hit == CANCEL_BUTTON || hit == OK_BUTTON) opeu_numdlg_press(game, service, hit);
        opeu_numdlg_highlight(game, service, shown);
        opeu_numdlg_highlight(game, service, hit);
        shown = hit;
      }
      if (opeu_mouse_left(game, service)) {
        if (hit == TITLE_BAR) {
          opeu_numdlg_drag(game, service);
          opeu_numdlg_bar(game, service, lo, hi, value);
          opeu_numdlg_show(game, service, value);
          left = Column(data);
        } else if (hit == CANCEL_BUTTON) {
          if (cancel) {
            value   = MENU_CANCELLED;
            running = false;
          }
        } else if (hit == OK_BUTTON) {
          if (lo <= value && hi >= value) running = false;
        } else if (hit <= LAST_DIGIT) {
          value = opeu_numdlg_increment(game, service, hit - FIRST_DIGIT, lo, hi, value);
          opeu_numdlg_bar(game, service, lo, hi, value);
          opeu_input_reset(game, service);
        }
      }
      if (opeu_mouse_right(game, service) && hit >= FIRST_DIGIT && hit <= LAST_DIGIT) {
        value = opeu_numdlg_decrement(game, service, hit - FIRST_DIGIT, lo, hi, value);
        opeu_numdlg_bar(game, service, lo, hi, value);
        opeu_input_reset(game, service);
      }
    }
    if (opeu_numdlg_outside(game, mouse.x, mouse.y)) {
      if (opeu_mouse_left(game, service)) {
        if (lo <= value && hi >= value) running = false;
      } else if (opeu_mouse_right(game, service) && cancel) {
        value   = MENU_CANCELLED;
        running = false;
      }
    }
  } while (running);
  opeu_mouse_hide(game, service);
  return static_cast<std::uint16_t>(value);
}

auto opeu_numdlg_run(Game& game, HostService service, std::uint16_t lo, std::uint16_t hi, int cancel) -> std::uint16_t {
  opeu_numdlg_open(game, service);
  auto const value{ opeu_numdlg_track(game, service, lo, hi, cancel) };
  opeu_numdlg_close(game);
  opeu_input_reset(game, service);
  return value;
}

// In the game the dialog runs from the keyboard module, which passes its own service down; this one serves direct
// callers.
auto svc_numdlg(Game& game, std::uint16_t call, std::uint16_t* words) -> std::uint16_t {
  switch (static_cast<InputCall>(call)) {
  case InputCall::MOUSE_INT86:
  case InputCall::TEXT_SAVE:
  case InputCall::TEXT_POSITION:
  case InputCall::TEXT_PRINT:
  case InputCall::TEXT_RESTORE:
  case InputCall::BITMAP_SAVE:
  case InputCall::BITMAP_RESTORE: return svc_menudraw(game, call, words);
  case InputCall::UI_FILL:        opeu_ui_fill(words[0], words[1], words[2], words[3], words[4]); break;
  case InputCall::UI_INVERT:      opeu_ui_highlight(words[0], words[1], words[2], words[3], words[4]); break;
  case InputCall::UI_LINE:        opeu_ui_line(words[0], words[1], words[2], words[3], words[4]); break;
  default:                        port_unhandled("numdlg", call, 7);
  }
  return 0;
}
