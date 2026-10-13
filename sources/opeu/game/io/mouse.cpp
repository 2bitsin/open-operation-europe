// The mouse services (INT 33h). input_mode is nonzero when a mouse is present. Mouse rows are offset by 40 pixels:
// positions report DX-40 and set or limit DX+40. Button predicates return -1 or 0 (BX bit 0 left, bit 1 right);
// opeu_turn_input_a is "any button".
#include "io/mouse.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "io/busy-wait.hpp"
#include "link/glue.hpp"

#include <array>

using namespace opeu::game;

namespace {

enum Function : std::uint16_t {
  SHOW    = 1,
  HIDE    = 2,
  STATUS  = 3, // position and buttons
  X_RANGE = 7, // CX..DX
  Y_RANGE = 8, // CX..DX
};

constexpr std::uint16_t ROW_OFFSET{ 40 };
constexpr std::uint16_t LEFT{ 1 };
constexpr std::uint16_t RIGHT{ 2 };
constexpr std::uint8_t  PRESSED{ 0x80 };

// What the driver reports: the buttons (bit 0 left, bit 1 right) and the position in its own rows.
struct DriverStatus {
  std::uint16_t buttons;
  std::uint16_t x;
  std::uint16_t row;
};

// INT 33h with AX = `function`, CX and DX; answers the registers AX, BX, CX and DX. The one driver call of the mouse:
// the typed helpers below are its call shapes.
auto MouseInterrupt(Function function, std::uint16_t cx, std::uint16_t dx) -> std::array<std::uint16_t, 7> {
  std::array<std::uint16_t, 7> registers{ function, 0, cx, dx };
  port_mouse_int33(registers.data());
  return registers;
}

auto ShowPointer(Game&) -> void {
  MouseInterrupt(SHOW, 0, 0);
}

auto HidePointer(Game&) -> void {
  MouseInterrupt(HIDE, 0, 0);
}

auto Status(Game&) -> DriverStatus {
  auto const registers{ MouseInterrupt(STATUS, 0, 0) };
  return { registers[1], registers[2], registers[3] };
}

// Limits the pointer to columns low..high.
auto LimitColumns(Game&, std::uint16_t low, std::uint16_t high) -> void {
  MouseInterrupt(X_RANGE, low, high);
}

// Limits the pointer to driver rows low..high.
auto LimitRows(Game&, std::uint16_t low, std::uint16_t high) -> void {
  MouseInterrupt(Y_RANGE, low, high);
}

auto Pressed(Game& game, std::uint16_t button) -> int {
  return (Status(game).buttons & button) ? -1 : 0;
}
} // namespace

void opeu_mouse_show(Game& game) {
  if (HasMouse(Data(game))) ShowPointer(game);
}

void opeu_mouse_hide(Game& game) {
  if (HasMouse(Data(game))) HidePointer(game);
}

auto opeu::game::detail::mouse::MousePosition(Game& game) -> MousePoint {
  if (!HasMouse(Data(game))) return { 0, 0 };
  auto const status{ Status(game) };
  return { status.x, static_cast<std::uint16_t>(status.row - ROW_OFFSET) };
}

// Stores the position at the DS words x and y, for the callers that pass their near addresses through a service.
void opeu_mouse_position(Game& game, std::uint16_t x, std::uint16_t y) {
  auto const position{ MousePosition(game) };
  SetWord(game, x, position.x);
  SetWord(game, y, position.y);
}

// Queries the driver even without a mouse.
auto opeu_mouse_left(Game& game) -> int {
  return Pressed(game, LEFT);
}

auto opeu_mouse_right(Game& game) -> int {
  return Pressed(game, RIGHT);
}

auto opeu_turn_input_a(Game& game) -> int {
  return opeu_mouse_left(game) || opeu_mouse_right(game);
}

// 0 without a mouse or a button; else 0x80 | 1 left | 2 right.
auto opeu_mouse_buttons(Game& game) -> std::uint8_t {
  if (!HasMouse(Data(game)) || !opeu_turn_input_a(game)) return 0;
  std::uint8_t buttons{ opeu_mouse_left(game) ? static_cast<std::uint8_t>(PRESSED | LEFT) : PRESSED };
  if (opeu_mouse_right(game)) buttons |= RIGHT;
  return buttons;
}

// Waits until every button is released.
void opeu_input_reset(Game& game) {
  if (HasMouse(Data(game))) WaitWhile([&] { return opeu_turn_input_a(game) != 0; });
}

void opeu_mouse_window(Game& game, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height) {
  LimitColumns(game, static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(width + x));
  LimitRows(game, static_cast<std::uint16_t>(y + ROW_OFFSET), static_cast<std::uint16_t>(y + height + ROW_OFFSET));
}
