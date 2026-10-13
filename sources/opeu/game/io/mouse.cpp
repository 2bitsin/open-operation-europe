// The mouse, through the machine's input. input_mode is nonzero when a mouse is present. The driver's rows are offset
// by 40 pixels: positions report y-40 and limit y+40. Button predicates return -1 or 0 (bit 0 left, bit 1 right);
// opeu_turn_input_a is "any button".
#include "io/mouse.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "io/busy-wait.hpp"

using namespace opeu::game;

namespace {

constexpr std::uint16_t ROW_OFFSET{ 40 };
constexpr std::uint16_t LEFT{ 1 };
constexpr std::uint16_t RIGHT{ 2 };
constexpr std::uint8_t  PRESSED{ 0x80 };

auto Pressed(Game& game, std::uint16_t button) -> int {
  return (game.Machine().Input().Mouse().buttons & button) ? -1 : 0;
}
} // namespace

void opeu_mouse_show(Game& game) {
  if (HasMouse(Data(game))) game.Machine().Input().ShowPointer();
}

void opeu_mouse_hide(Game& game) {
  if (HasMouse(Data(game))) game.Machine().Input().HidePointer();
}

auto opeu::game::detail::mouse::MousePosition(Game& game) -> ScreenPoint {
  if (!HasMouse(Data(game))) return { 0, 0 };
  auto const mouse{ game.Machine().Input().Mouse() };
  return { mouse.x, static_cast<std::uint16_t>(mouse.y - ROW_OFFSET) };
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

auto opeu::game::detail::mouse::LimitMouse(Game& game, MouseArea const& area) -> void {
  auto const [x, y, width, height]{ area };
  game.Machine().Input().LimitPointerX({ static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(width + x) });
  game.Machine().Input().LimitPointerY({ static_cast<std::uint16_t>(y + ROW_OFFSET), static_cast<std::uint16_t>(y + height + ROW_OFFSET) });
}
