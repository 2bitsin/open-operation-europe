// The mouse services (INT 33h).
#pragma once

#include "common/events.hpp"
#include "common/screen-point.hpp"

#include <cstdint>

void opeu_mouse_show(Game& game);
void opeu_mouse_hide(Game& game);
auto opeu_mouse_left(Game& game) -> int;
auto opeu_mouse_right(Game& game) -> int;
auto opeu_turn_input_a(Game& game) -> int;
auto opeu_mouse_buttons(Game& game) -> std::uint8_t;
void opeu_input_reset(Game& game);

namespace opeu::game::detail::mouse {
// The area the pointer is kept in: from (x, y), width and height pixels on (screen rows, before the driver's offset).
struct MouseArea {
  std::int16_t x;
  std::int16_t y;
  std::int16_t width;
  std::int16_t height;
};

// Keeps the pointer inside the area.
auto LimitMouse(Game& game, MouseArea const& area) -> void;

// The mouse position; 0, 0 without a mouse.
auto MousePosition(Game& game) -> ScreenPoint;
} // namespace opeu::game::detail::mouse

namespace opeu::game {
using detail::mouse::LimitMouse;
using detail::mouse::MouseArea;
using detail::mouse::MousePosition;
} // namespace opeu::game
