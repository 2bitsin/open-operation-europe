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
#include "io/menudraw.hpp"
#include "io/mouse.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {

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

// The place values of the five digits; the first takes everything above 9999.
constexpr std::array<std::int32_t, 5> PLACES{ TOP_PLACE, 1000, 100, 10, 1 };

auto Column(DataSegment const& data) -> std::uint16_t {
  return static_cast<std::uint16_t>(data.numdlg_x << 3);
}

auto IsButton(std::uint8_t hit) -> bool {
  return hit == CANCEL_BUTTON || hit == OK_BUTTON;
}

auto IsDigit(std::uint8_t hit) -> bool {
  return hit >= FIRST_DIGIT && hit <= LAST_DIGIT;
}

// An outline of the dialog's size with its top-left corner at 8-pixel column `column` and pixel row `row`.
auto Outline(Game& game, std::uint16_t column, std::uint16_t row) -> void {
  std::uint16_t const x = column << 3;
  opeu_ui_outline(game, x, row, x + 0x8f, row + 0x5f, OUTLINE_COLOUR);
}

// Redraws button n's picture `tile`.
void opeu_numdlg_button(Game& game, std::uint8_t n, std::uint16_t tile) {
  auto const& data{ Data(game) };
  opeu_mouse_hide(game);
  game.Machine().Display().DrawColumns({ static_cast<uint16_t>(n * 5 + data.numdlg_x + 7), static_cast<uint16_t>(data.numdlg_y + 40), 4, 16 },
                                       { 0, FarBytes(game, data.message_picture_segment, tile) });
  opeu_mouse_show(game);
}

// Button 1 (cancel) or 2 (OK) pressed.
void opeu_numdlg_press(Game& game, std::uint8_t n) {
  opeu_numdlg_button(game, n - 1, n == CANCEL_BUTTON ? CANCEL_DOWN : OK_DOWN);
}

void opeu_numdlg_release(Game& game, std::uint8_t n) {
  opeu_numdlg_button(game, n - 1, n == CANCEL_BUTTON ? CANCEL_UP : OK_UP);
}

// Saves the covered area, draws the dialog and both buttons.
void opeu_numdlg_open(Game& game) {
  auto const& data{ Data(game) };
  game.Machine().Display().SaveColumns({ data.numdlg_x, data.numdlg_y, 0x12, 0x60 }, FarBytes(game, data.numdlg_save_segment, 0));
  game.Machine().Display().DrawColumns({ data.numdlg_x, data.numdlg_y, 0x12, 0x60 }, { 0, FarBytes(game, data.message_picture_segment, 0) });
  opeu_numdlg_release(game, CANCEL_BUTTON);
  opeu_numdlg_release(game, OK_BUTTON);
}

void opeu_numdlg_close(Game& game) {
  auto const& data{ Data(game) };
  game.Machine().Display().DrawColumns({ data.numdlg_x, data.numdlg_y, 0x12, 0x60 }, { 0, FarBytes(game, data.numdlg_save_segment, 0) });
}

void opeu_numdlg_digit(Game& game, std::uint8_t i, std::uint8_t digit) {
  auto const& data{ Data(game) };
  opeu_context_save(game);
  opeu_army_position(game, static_cast<std::int16_t>(i * 3 + data.numdlg_x + 2), static_cast<std::int16_t>(data.numdlg_y + 0x40));
  opeu_cursor_print(game, std::array<FormatArgument, 3>{ Near(data, data.numdlg_format), digit, 0 });
  opeu_context_restore(game);
}

// Splits a value into the five digits (the first takes everything above 9999).
void opeu_numdlg_split(Game& game, std::int32_t value) {
  std::ranges::transform(PLACES, Data(game).numdlg_digits.begin(), [&](std::int32_t place) {
    std::uint16_t const digit  = value / place;
    value                     -= digit * place;
    return static_cast<std::uint8_t>(digit);
  });
}

auto opeu_numdlg_value(Game& game) -> std::int32_t {
  auto const& digits{ Data(game).numdlg_digits };
  return std::ranges::fold_left(std::views::zip(digits, PLACES), std::int32_t{ 0 }, [](std::int32_t value, auto const& pair) {
    auto const [digit, place]{ pair };
    return value + digit * place;
  });
}

void opeu_numdlg_show(Game& game, std::int32_t value) {
  opeu_numdlg_split(game, value);
  opeu_mouse_hide(game);
  std::ranges::for_each(std::views::iota(std::uint8_t{ 0 }, static_cast<std::uint8_t>(PLACES.size())), [&](std::uint8_t c) { opeu_numdlg_digit(game, c, Data(game).numdlg_digits[c]); });
  opeu_mouse_show(game);
}

// What the mouse is on: TITLE_BAR, CANCEL_BUTTON, OK_BUTTON, a digit (FIRST_DIGIT..LAST_DIGIT) or NOTHING.
auto opeu_numdlg_hit(Game& game, ScreenPoint const& mouse) -> std::uint8_t {
  auto const&         data{ Data(game) };
  std::uint8_t        hit{ NOTHING };
  std::uint16_t const x{ Column(data) };
  if (static_cast<std::uint16_t>(x + 0x10) > mouse.x || static_cast<std::uint16_t>(data.numdlg_y + 0x20) > mouse.y) return NOTHING;
  std::uint16_t row = mouse.y - data.numdlg_y - 0x10;
  if (row % 24 < 16) {
    row                  /= 24;
    std::uint16_t column  = mouse.x - x - 0x10;
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
void opeu_numdlg_highlight(Game& game, std::uint8_t n) {
  if (n == NOTHING || IsButton(n)) return;
  std::uint16_t x{ 2 };
  std::uint16_t y{ 0x28 };
  std::uint16_t width{ 4 };
  if (n != TITLE_BAR) {
    x     = (n - FIRST_DIGIT) * 3 + 2;
    y     = 0x40;
    width = 2;
  }
  auto const& data{ Data(game) };
  opeu_mouse_hide(game);
  game.Machine().Display().Highlight({ static_cast<uint16_t>(data.numdlg_x + x), static_cast<uint16_t>(data.numdlg_y + y), width, 0x10 }, INVERT_MODE);
  opeu_mouse_show(game);
}

// The dialog's top-left corner while the mouse drags it by the grab point: an 8-pixel column and a pixel row.
auto DragCorner(ScreenPoint mouse, ScreenPoint const& grab) -> ScreenPoint {
  if (mouse.x > 0x212) mouse.x = 0x1f4;
  if (mouse.y > 0x154) mouse.y = 0x154;
  return { static_cast<std::uint16_t>(static_cast<std::uint16_t>(mouse.x - grab.x) >> 3), static_cast<std::uint16_t>(mouse.y - grab.y) };
}

// Moves the dialog with an outline while the left button is held.
void opeu_numdlg_drag(Game& game) {
  auto& data{ Data(game) };
  opeu_mouse_hide(game);
  opeu_numdlg_close(game);
  opeu_mouse_show(game);
  auto const mouse{ MousePosition(game) };
  // the grab point within the dialog; the mouse stays where the dialog stays on the screen
  ScreenPoint const grab{ static_cast<std::uint16_t>(mouse.x - Column(data)), static_cast<std::uint16_t>(mouse.y - data.numdlg_y) };
  LimitMouse(
      game,
      { .x = static_cast<std::int16_t>(grab.x), .y = static_cast<std::int16_t>(grab.y), .width = static_cast<std::int16_t>(grab.x + 0x1f0), .height = static_cast<std::int16_t>(grab.y + 0x130) });
  ScreenPoint shown{ 0xffff, 0xffff };
  do {
    auto const corner{ DragCorner(MousePosition(game), grab) };
    if (corner.x != shown.x || corner.y != shown.y) {
      if (shown.x != 0xffff) {
        opeu_mouse_hide(game);
        Outline(game, shown.x, shown.y);
        opeu_mouse_show(game);
      }
      opeu_mouse_hide(game);
      Outline(game, corner.x, corner.y);
      opeu_mouse_show(game);
      shown = corner;
    }
  } while (opeu_mouse_left(game));
  data.numdlg_x = shown.x;
  data.numdlg_y = shown.y;
  opeu_mouse_hide(game);
  Outline(game, shown.x, shown.y);
  opeu_numdlg_open(game);
  opeu_mouse_show(game);
  LimitMouse(game, { .x = 0, .y = 0, .width = 0x27f, .height = 0x18f });
}

void opeu_numdlg_refresh(Game& game, std::uint8_t n) {
  opeu_mouse_hide(game);
  opeu_numdlg_digit(game, n, Data(game).numdlg_digits[n]);
  opeu_numdlg_highlight(game, n + FIRST_DIGIT);
  opeu_mouse_show(game);
}

// 0 inside the dialog's lower body x..x+8Fh, y+20h..y+5Fh.
auto opeu_numdlg_outside(Game& game, ScreenPoint const& mouse) -> std::uint16_t {
  auto const&         data{ Data(game) };
  std::uint16_t const x{ Column(data) };
  if (x <= mouse.x && static_cast<std::uint16_t>(x + 0x90) > mouse.x && static_cast<std::uint16_t>(data.numdlg_y + 0x20) <= mouse.y && static_cast<std::uint16_t>(data.numdlg_y + 0x60) > mouse.y)
    return 0;
  return 1;
}

// One run of the dialog on the value range lo..hi: the value shown, the button or digit under the mouse.
class NumericDialog {
public:
  NumericDialog(Game& game, ValueRange range, int cancel) : _game{ game }, _range{ range }, _cancel{ cancel }, _value{ range.lo }, _left{ Column(Data(game)) } { }

  // Runs until a value in range is accepted or (when allowed) the dialog is cancelled; answers the value or 0xffff.
  auto Track() -> std::uint16_t {
    opeu_input_reset(_game);
    opeu_mouse_show(_game);
    Bar(_range.lo);
    opeu_numdlg_show(_game, _range.lo);
    do {
      auto const mouse{ MousePosition(_game) };
      if (static_cast<std::uint16_t>(Data(_game).numdlg_y + 0x20) > mouse.y)
        Slide(mouse.x);
      else
        Point(mouse);
      ClickOutside(mouse);
    } while (_running);
    opeu_mouse_hide(_game);
    return static_cast<std::uint16_t>(_value);
  }

private:
  // The slider: 0..BAR_WIDTH pixels filled for lo..hi.
  auto Bar(std::int32_t value) -> void {
    auto const&         data{ Data(_game) };
    std::uint16_t const x{ Column(data) };
    std::uint16_t const y{ data.numdlg_y };
    std::uint16_t const range = _range.hi - _range.lo;
    std::uint16_t       filled;
    if (value < _range.lo)
      filled = 0;
    else if (value > _range.hi || range == 0)
      filled = BAR_WIDTH;
    else
      filled = opeu_mul_div(static_cast<std::uint16_t>(value) - _range.lo, BAR_WIDTH, range);
    opeu_mouse_hide(_game);
    if (filled)
      _game.Machine().Display().Fill(
          {
              { static_cast<uint16_t>(x + 0x10),         static_cast<uint16_t>(y + 0x14) },
              { static_cast<uint16_t>(x + filled + 0xf), static_cast<uint16_t>(y + 0x1b) }
      },
          2);
    if (filled != BAR_WIDTH)
      _game.Machine().Display().Fill(
          {
              { static_cast<uint16_t>(x + filled + 0x10), static_cast<uint16_t>(y + 0x14) },
              { static_cast<uint16_t>(x + 0x7f),          static_cast<uint16_t>(y + 0x1b) }
      },
          0);
    opeu_mouse_show(_game);
  }

  auto Show() -> void {
    Bar(_value);
    opeu_numdlg_show(_game, _value);
  }

  // The pointer on the slider sets the value its position stands for.
  auto Slide(std::uint16_t x) -> void {
    auto const previous{ _value };
    if (static_cast<std::uint16_t>(_left + 0x10) > x)
      _value = _range.lo;
    else if (static_cast<std::uint16_t>(_left + 0x7f) <= x)
      _value = _range.hi;
    else
      _value = static_cast<std::uint16_t>(opeu_mul_div(_range.hi - _range.lo + 1, x - _left - 0xf, BAR_WIDTH) + _range.lo);
    if (_value != previous) Show();
  }

  // The pointer on the dialog's body: a button under it is pressed, a digit or the title bar inverted; the left
  // button drags, cancels, accepts or counts a digit up, the right button counts it down.
  auto Point(ScreenPoint const& mouse) -> void {
    auto const hit{ opeu_numdlg_hit(_game, mouse) };
    if (hit != _shown) {
      if (IsButton(_shown)) opeu_numdlg_release(_game, _shown);
      if (IsButton(hit)) opeu_numdlg_press(_game, hit);
      opeu_numdlg_highlight(_game, _shown);
      opeu_numdlg_highlight(_game, hit);
      _shown = hit;
    }
    if (opeu_mouse_left(_game)) LeftClick(hit);
    if (opeu_mouse_right(_game) && IsDigit(hit)) Step(hit - FIRST_DIGIT, Decremented(hit - FIRST_DIGIT));
  }

  auto LeftClick(std::uint8_t hit) -> void {
    if (hit == TITLE_BAR) {
      opeu_numdlg_drag(_game);
      Show();
      _left = Column(Data(_game));
    } else if (hit == CANCEL_BUTTON) {
      if (_cancel) Cancel();
    } else if (hit == OK_BUTTON) {
      AcceptInRange();
    } else if (hit <= LAST_DIGIT) {
      Step(hit - FIRST_DIGIT, Incremented(hit - FIRST_DIGIT));
    }
  }

  // Outside the body the left button accepts, the right one cancels when allowed.
  auto ClickOutside(ScreenPoint const& mouse) -> void {
    if (!opeu_numdlg_outside(_game, mouse)) return;
    if (opeu_mouse_left(_game))
      AcceptInRange();
    else if (opeu_mouse_right(_game) && _cancel)
      Cancel();
  }

  auto AcceptInRange() -> void {
    if (_range.lo <= _value && _range.hi >= _value) _running = false;
  }

  auto Cancel() -> void {
    _value   = MENU_CANCELLED;
    _running = false;
  }

  // Digit n of the value up, wrapping after 6 (the first) or 9.
  auto Incremented(std::uint8_t n) -> std::uint8_t {
    opeu_numdlg_split(_game, _value);
    auto const         digit{ Data(_game).numdlg_digits[n] };
    std::uint8_t const top{ static_cast<std::uint8_t>(n == 0 ? 6 : 9) };
    return digit + 1 > top ? 0 : static_cast<std::uint8_t>(digit + 1);
  }

  // Digit n down, wrapping from 0 to 6 (the first) or 9.
  auto Decremented(std::uint8_t n) -> std::uint8_t {
    opeu_numdlg_split(_game, _value);
    auto const digit{ Data(_game).numdlg_digits[n] };
    return static_cast<std::uint8_t>((digit == 0 ? (n == 0 ? 7 : 10) : digit) - 1);
  }

  // Sets digit n, clamps the value to the range (showing it when clamped) and redraws the digit and the bar.
  auto Step(std::uint8_t n, std::uint8_t digit) -> void {
    Data(_game).numdlg_digits[n] = digit;
    _value                       = std::clamp(opeu_numdlg_value(_game), std::int32_t{ _range.lo }, std::int32_t{ _range.hi });
    if (_value != opeu_numdlg_value(_game)) opeu_numdlg_show(_game, _value);
    opeu_numdlg_refresh(_game, n);
    Bar(_value);
    opeu_input_reset(_game);
  }

  Game&         _game;
  ValueRange    _range;
  int           _cancel;
  std::int32_t  _value;
  std::uint16_t _left;
  std::uint8_t  _shown{ NOTHING };
  bool          _running{ true };
};
} // namespace

auto opeu_numdlg_run(Game& game, ValueRange range, int cancel) -> std::uint16_t {
  opeu_numdlg_open(game);
  auto const value{
    NumericDialog{ game, range, cancel }
    .Track()
  };
  opeu_numdlg_close(game);
  opeu_input_reset(game);
  return value;
}
