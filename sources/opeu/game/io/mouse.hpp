// The mouse services (INT 33h).
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_mouse_reset(Game& game) -> std::uint16_t;
void opeu_mouse_show(Game& game);
void opeu_mouse_hide(Game& game);
void opeu_mouse_position(Game& game, std::uint16_t x, std::uint16_t y);
void opeu_mouse_move(Game& game, std::uint16_t x, std::uint16_t y);
auto opeu_mouse_left(Game& game) -> int;
auto opeu_mouse_right(Game& game) -> int;
auto opeu_turn_input_a(Game& game) -> int;
auto opeu_mouse_buttons(Game& game) -> std::uint8_t;
void opeu_input_reset(Game& game);
auto opeu_mouse_wait_click(Game& game) -> std::uint8_t;
void opeu_mouse_window(Game& game, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height);

namespace opeu::game::detail::mouse {
// A mouse position in screen pixels.
struct MousePoint {
  std::uint16_t x;
  std::uint16_t y;
};

// The mouse position; 0, 0 without a mouse.
auto MousePosition(Game& game) -> MousePoint;
} // namespace opeu::game::detail::mouse

namespace opeu::game {
using detail::mouse::MousePoint;
using detail::mouse::MousePosition;
} // namespace opeu::game
