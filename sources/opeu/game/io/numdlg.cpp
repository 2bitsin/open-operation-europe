// The five-digit numeric entry dialog with a slider bar. numdlg_x and numdlg_y place the dialog (an 8-pixel column
// and a pixel row), numdlg_digits holds its five decimal digits (the first may exceed 9). Its pictures are in the
// message picture segment; the graphics it covers are saved in numdlg_save_segment.
#include "io/numdlg.hpp"

#include "common/data-segment.hpp"
#include "common/host.hpp"
#include "common/memory.hpp"
#include "core/emsmap.hpp"
#include "core/numeric.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/input-call.hpp"
#include "io/menudraw.hpp"
#include "io/mouse.hpp"
#include "port.hpp"
#include "services.hpp"

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
constexpr std::uint16_t CANCELLED{ 0xffff };
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

auto OutlineLine(std::uint8_t* ds, std::uint8_t* memory, std::uint16_t id, std::uint16_t const* args,
                 std::uint16_t count) -> std::uint32_t {
  constexpr std::uint16_t LINE_EVENT{ 5 };
  if (id != LINE_EVENT || count != 5) return 0;
  auto const* bridge{ reinterpret_cast<OutlineBridge const*>(memory) };
  Input(ds, bridge->service, InputCall::UI_LINE, args[0], args[1], args[2], args[3], args[4]);
  return 0;
}

auto Outline(std::uint8_t* ds, HostService service, std::uint16_t x1, std::uint16_t y1, std::uint16_t x2,
             std::uint16_t y2) -> void {
  OutlineBridge bridge{ service };
  UiHost        host{ reinterpret_cast<std::uint8_t*>(&bridge), 0, 0, 0, OutlineLine };
  opeu_ui_outline(ds, &host, x1, y1, x2, y2, OUTLINE_COLOUR);
}

// Clamps a dialog value to lo..hi, showing the clamped value.
auto Clamped(std::uint8_t* ds, HostService service, std::uint16_t lo, std::uint16_t hi, std::int32_t value)
    -> std::int32_t {
  if (value < lo) {
    value = lo;
    opeu_numdlg_show(ds, service, value);
  } else if (value > hi) {
    value = hi;
    opeu_numdlg_show(ds, service, value);
  }
  return value;
}
} // namespace

// Redraws button n's picture `tile`.
void opeu_numdlg_button(std::uint8_t* ds, HostService service, std::uint8_t n, std::uint16_t tile) {
  auto const& data{ Data(ds) };
  opeu_mouse_hide(ds, service);
  Input(ds, service, InputCall::EMS_MAP);
  Input(ds, service, InputCall::BITMAP_RESTORE, n * 5 + data.numdlg_x + 7, data.numdlg_y + 40, 4, 16, 0,
        data.message_picture_segment, tile);
  opeu_mouse_show(ds, service);
}

// Button 1 (cancel) or 2 (OK) pressed.
void opeu_numdlg_press(std::uint8_t* ds, HostService service, std::uint8_t n) {
  opeu_numdlg_button(ds, service, n - 1, n == CANCEL_BUTTON ? CANCEL_DOWN : OK_DOWN);
}

void opeu_numdlg_release(std::uint8_t* ds, HostService service, std::uint8_t n) {
  opeu_numdlg_button(ds, service, n - 1, n == CANCEL_BUTTON ? CANCEL_UP : OK_UP);
}

// Saves the covered area, draws the dialog and both buttons.
void opeu_numdlg_open(std::uint8_t* ds, HostService service) {
  auto const& data{ Data(ds) };
  Input(ds, service, InputCall::BITMAP_SAVE, data.numdlg_x, data.numdlg_y, 0x12, 0x60, data.numdlg_save_segment, 0);
  Input(ds, service, InputCall::EMS_MAP);
  Input(ds, service, InputCall::BITMAP_RESTORE, data.numdlg_x, data.numdlg_y, 0x12, 0x60, 0,
        data.message_picture_segment, 0);
  opeu_numdlg_release(ds, service, CANCEL_BUTTON);
  opeu_numdlg_release(ds, service, OK_BUTTON);
}

void opeu_numdlg_close(std::uint8_t* ds, HostService service) {
  auto const& data{ Data(ds) };
  Input(ds, service, InputCall::BITMAP_RESTORE, data.numdlg_x, data.numdlg_y, 0x12, 0x60, 0, data.numdlg_save_segment,
        0);
}

void opeu_numdlg_digit(std::uint8_t* ds, HostService service, std::uint8_t i, std::uint8_t digit) {
  auto const& data{ Data(ds) };
  Input(ds, service, InputCall::TEXT_SAVE);
  Input(ds, service, InputCall::TEXT_POSITION, i * 3 + data.numdlg_x + 2, data.numdlg_y + 0x40);
  Input(ds, service, InputCall::TEXT_PRINT, Near(data, data.numdlg_format), digit);
  Input(ds, service, InputCall::TEXT_RESTORE);
}

// Splits a value into the five digits (the first takes everything above 9999).
void opeu_numdlg_split(std::uint8_t* ds, HostService, std::int32_t value) {
  auto&        digits{ Data(ds).numdlg_digits };
  std::int32_t place{ TOP_PLACE };
  for (auto c{ 0 }; c < DIGIT_COUNT; c++) {
    std::uint16_t const digit  = value / place;
    digits[c]                  = static_cast<std::uint8_t>(digit);
    value                     -= digit * place;
    place                     /= 10;
  }
}

auto opeu_numdlg_value(std::uint8_t* ds, HostService) -> std::int32_t {
  auto const&  digits{ Data(ds).numdlg_digits };
  std::int32_t place{ TOP_PLACE };
  std::int32_t value{ 0 };
  for (auto c{ 0 }; c < DIGIT_COUNT; c++) {
    value += digits[c] * place;
    place /= 10;
  }
  return value;
}

void opeu_numdlg_show(std::uint8_t* ds, HostService service, std::int32_t value) {
  opeu_numdlg_split(ds, service, value);
  opeu_mouse_hide(ds, service);
  for (auto c{ 0 }; c < DIGIT_COUNT; c++) opeu_numdlg_digit(ds, service, c, Data(ds).numdlg_digits[c]);
  opeu_mouse_show(ds, service);
}

// The slider: 0..BAR_WIDTH pixels filled for lo..hi.
void opeu_numdlg_bar(std::uint8_t* ds, HostService service, std::uint16_t lo, std::uint16_t hi, std::int32_t value) {
  auto const&         data{ Data(ds) };
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
  opeu_mouse_hide(ds, service);
  if (filled) Input(ds, service, InputCall::UI_FILL, x + 0x10, y + 0x14, x + filled + 0xf, y + 0x1b, 2);
  if (filled != BAR_WIDTH) Input(ds, service, InputCall::UI_FILL, x + filled + 0x10, y + 0x14, x + 0x7f, y + 0x1b, 0);
  opeu_mouse_show(ds, service);
}

// What the mouse at (mx, my) is on: TITLE_BAR, CANCEL_BUTTON, OK_BUTTON, a digit (FIRST_DIGIT..LAST_DIGIT) or
// NOTHING.
auto opeu_numdlg_hit(std::uint8_t* ds, HostService, std::uint16_t mx, std::uint16_t my) -> std::uint8_t {
  auto const&         data{ Data(ds) };
  std::uint8_t        hit{ NOTHING };
  std::uint16_t const x{ Column(data) };
  if (static_cast<std::uint16_t>(x + 0x10) > mx || static_cast<std::uint16_t>(data.numdlg_y + 0x20) > my)
    return NOTHING;
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
void opeu_numdlg_highlight(std::uint8_t* ds, HostService service, std::uint8_t n) {
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
  auto const& data{ Data(ds) };
  opeu_mouse_hide(ds, service);
  Input(ds, service, InputCall::UI_INVERT, data.numdlg_x + x, data.numdlg_y + y, width, 0x10, INVERT_MODE);
  opeu_mouse_show(ds, service);
}

// Moves the dialog with an outline while the left button is held. The mouse position words are at scratch+4 (x)
// and scratch+6 (y).
void opeu_numdlg_drag(std::uint8_t* ds, HostService service, std::uint16_t scratch) {
  auto&      data{ Data(ds) };
  auto const x_at{ static_cast<std::uint16_t>(scratch + 4) };
  auto const y_at{ static_cast<std::uint16_t>(scratch + 6) };
  opeu_mouse_hide(ds, service);
  opeu_numdlg_close(ds, service);
  opeu_mouse_show(ds, service);
  opeu_mouse_position(ds, service, x_at, y_at);
  // the grab point within the dialog; the mouse stays where the dialog stays on the screen
  std::uint16_t const grab_x = Word(ds, x_at) - Column(data);
  std::uint16_t const grab_y = Word(ds, y_at) - data.numdlg_y;
  opeu_mouse_window(ds, service, grab_x, grab_y, grab_x + 0x1f0, grab_y + 0x130);
  std::uint16_t column{ 0xffff };
  std::uint16_t row{ 0xffff };
  do {
    opeu_mouse_position(ds, service, x_at, y_at);
    std::uint16_t x{ Word(ds, x_at) };
    std::uint16_t y{ Word(ds, y_at) };
    if (x > 0x212) x = 0x1f4;
    if (y > 0x154) y = 0x154;
    x  -= grab_x;
    y  -= grab_y;
    x >>= 3;
    SetWord(ds, x_at, x);
    SetWord(ds, y_at, y);
    if (x != column || y != row) {
      if (column != 0xffff) {
        opeu_mouse_hide(ds, service);
        Outline(ds, service, column << 3, row, (column << 3) + 0x8f, row + 0x5f);
        opeu_mouse_show(ds, service);
      }
      opeu_mouse_hide(ds, service);
      Outline(ds, service, x << 3, y, (x << 3) + 0x8f, y + 0x5f);
      opeu_mouse_show(ds, service);
      column = x;
      row    = y;
    }
  } while (opeu_mouse_left(ds, service));
  data.numdlg_x = column;
  data.numdlg_y = row;
  opeu_mouse_hide(ds, service);
  Outline(ds, service, column << 3, row, (column << 3) + 0x8f, row + 0x5f);
  opeu_numdlg_open(ds, service);
  opeu_mouse_show(ds, service);
  opeu_mouse_window(ds, service, 0, 0, 0x27f, 0x18f);
}

void opeu_numdlg_refresh(std::uint8_t* ds, HostService service, std::uint8_t n) {
  opeu_mouse_hide(ds, service);
  opeu_numdlg_digit(ds, service, n, Data(ds).numdlg_digits[n]);
  opeu_numdlg_highlight(ds, service, n + FIRST_DIGIT);
  opeu_mouse_show(ds, service);
}

// Digit n goes up, wrapping after 6 (the first) or 9; the value is clamped to lo..hi.
auto opeu_numdlg_increment(std::uint8_t* ds, HostService service, std::uint8_t n, std::uint16_t lo, std::uint16_t hi,
                           std::int32_t value) -> std::int32_t {
  opeu_numdlg_split(ds, service, value);
  auto&              digit{ Data(ds).numdlg_digits[n] };
  std::uint8_t const top{ static_cast<std::uint8_t>(n == 0 ? 6 : 9) };
  if (++digit > top) digit = 0;
  value = Clamped(ds, service, lo, hi, opeu_numdlg_value(ds, service));
  opeu_numdlg_refresh(ds, service, n);
  return value;
}

// Digit n goes down, wrapping from 0 to 6 (the first) or 9.
auto opeu_numdlg_decrement(std::uint8_t* ds, HostService service, std::uint8_t n, std::uint16_t lo, std::uint16_t hi,
                           std::int32_t value) -> std::int32_t {
  opeu_numdlg_split(ds, service, value);
  auto& digit{ Data(ds).numdlg_digits[n] };
  if (digit == 0) digit = n == 0 ? 7 : 10;
  digit--;
  value = Clamped(ds, service, lo, hi, opeu_numdlg_value(ds, service));
  opeu_numdlg_refresh(ds, service, n);
  return value;
}

// 0 inside the dialog's lower body x..x+8Fh, y+20h..y+5Fh.
auto opeu_numdlg_outside(std::uint8_t* ds, HostService, std::uint16_t mx, std::uint16_t my) -> std::uint16_t {
  auto const&         data{ Data(ds) };
  std::uint16_t const x{ Column(data) };
  if (x <= mx && static_cast<std::uint16_t>(x + 0x90) > mx && static_cast<std::uint16_t>(data.numdlg_y + 0x20) <= my
      && static_cast<std::uint16_t>(data.numdlg_y + 0x60) > my)
    return 0;
  return 1;
}

// Runs the dialog until a value in lo..hi is accepted or (when allowed) the dialog is cancelled; returns the value or
// 0xffff. The mouse position words are at scratch (x) and scratch+2 (y); dragging uses scratch+4..+7.
auto opeu_numdlg_track(std::uint8_t* ds, HostService service, std::uint16_t lo, std::uint16_t hi, int cancel,
                       std::uint16_t scratch) -> std::uint16_t {
  auto const&  data{ Data(ds) };
  auto const   x_at{ scratch };
  auto const   y_at{ static_cast<std::uint16_t>(scratch + 2) };
  std::uint8_t shown{ NOTHING };
  std::int32_t value{ lo };
  bool         running{ true };
  opeu_input_reset(ds, service);
  opeu_mouse_show(ds, service);
  opeu_numdlg_bar(ds, service, lo, hi, lo);
  opeu_numdlg_show(ds, service, lo);
  std::uint16_t left{ Column(data) };
  do {
    opeu_mouse_position(ds, service, x_at, y_at);
    if (static_cast<std::uint16_t>(data.numdlg_y + 0x20) > Word(ds, y_at)) {
      // on the slider
      auto const previous{ value };
      auto const x{ Word(ds, x_at) };
      if (static_cast<std::uint16_t>(left + 0x10) > x)
        value = lo;
      else if (static_cast<std::uint16_t>(left + 0x7f) <= x)
        value = hi;
      else
        value = static_cast<std::uint16_t>(opeu_mul_div(hi - lo + 1, x - left - 0xf, BAR_WIDTH) + lo);
      if (value != previous) {
        opeu_numdlg_bar(ds, service, lo, hi, value);
        opeu_numdlg_show(ds, service, value);
      }
    } else {
      auto const hit{ opeu_numdlg_hit(ds, service, Word(ds, x_at), Word(ds, y_at)) };
      if (hit != shown) {
        if (shown == CANCEL_BUTTON || shown == OK_BUTTON) opeu_numdlg_release(ds, service, shown);
        if (hit == CANCEL_BUTTON || hit == OK_BUTTON) opeu_numdlg_press(ds, service, hit);
        opeu_numdlg_highlight(ds, service, shown);
        opeu_numdlg_highlight(ds, service, hit);
        shown = hit;
      }
      if (opeu_mouse_left(ds, service)) {
        if (hit == TITLE_BAR) {
          opeu_numdlg_drag(ds, service, scratch);
          opeu_numdlg_bar(ds, service, lo, hi, value);
          opeu_numdlg_show(ds, service, value);
          left = Column(data);
        } else if (hit == CANCEL_BUTTON) {
          if (cancel) {
            value   = CANCELLED;
            running = false;
          }
        } else if (hit == OK_BUTTON) {
          if (lo <= value && hi >= value) running = false;
        } else if (hit <= LAST_DIGIT) {
          value = opeu_numdlg_increment(ds, service, hit - FIRST_DIGIT, lo, hi, value);
          opeu_numdlg_bar(ds, service, lo, hi, value);
          opeu_input_reset(ds, service);
        }
      }
      if (opeu_mouse_right(ds, service) && hit >= FIRST_DIGIT && hit <= LAST_DIGIT) {
        value = opeu_numdlg_decrement(ds, service, hit - FIRST_DIGIT, lo, hi, value);
        opeu_numdlg_bar(ds, service, lo, hi, value);
        opeu_input_reset(ds, service);
      }
    }
    if (opeu_numdlg_outside(ds, service, Word(ds, x_at), Word(ds, y_at))) {
      if (opeu_mouse_left(ds, service)) {
        if (lo <= value && hi >= value) running = false;
      } else if (opeu_mouse_right(ds, service) && cancel) {
        value   = CANCELLED;
        running = false;
      }
    }
  } while (running);
  opeu_mouse_hide(ds, service);
  return static_cast<std::uint16_t>(value);
}

auto opeu_numdlg_run(std::uint8_t* ds, HostService service, std::uint16_t lo, std::uint16_t hi, int cancel,
                     std::uint16_t scratch) -> std::uint16_t {
  opeu_numdlg_open(ds, service);
  auto const value{ opeu_numdlg_track(ds, service, lo, hi, cancel, scratch) };
  opeu_numdlg_close(ds, service);
  opeu_input_reset(ds, service);
  return value;
}

// In the game the dialog runs from the keyboard module, which passes its own service down; this one serves direct
// callers.
auto svc_numdlg(std::uint8_t* ds, std::uint16_t call, std::uint16_t* words) -> std::uint16_t {
  switch (static_cast<InputCall>(call)) {
  case InputCall::MOUSE_INT86:
  case InputCall::TEXT_SAVE:
  case InputCall::TEXT_POSITION:
  case InputCall::TEXT_PRINT:
  case InputCall::TEXT_RESTORE:
  case InputCall::BITMAP_SAVE:
  case InputCall::BITMAP_RESTORE: return svc_menudraw(ds, call, words);
  case InputCall::UI_FILL:        opeu_ui_fill(words[0], words[1], words[2], words[3], words[4]); break;
  case InputCall::UI_INVERT:      opeu_ui_highlight(words[0], words[1], words[2], words[3], words[4]); break;
  case InputCall::EMS_MAP:        opeu_ems_map_dialog(ds, svc_emsmap); break;
  case InputCall::UI_LINE:        opeu_ui_line(words[0], words[1], words[2], words[3], words[4]); break;
  default:                        port_unhandled("numdlg", call, 7);
  }
  return 0;
}
