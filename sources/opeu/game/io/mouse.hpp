// The mouse services (INT 33h).
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_mouse_reset(Game& game, HostService service) -> std::uint16_t;
void opeu_mouse_show(Game& game, HostService service);
void opeu_mouse_hide(Game& game, HostService service);
void opeu_mouse_position(Game& game, HostService service, std::uint16_t x, std::uint16_t y);
void opeu_mouse_move(Game& game, HostService service, std::uint16_t x, std::uint16_t y);
auto opeu_mouse_left(Game& game, HostService service) -> int;
auto opeu_mouse_right(Game& game, HostService service) -> int;
auto opeu_turn_input_a(Game& game, HostService service) -> int;
auto opeu_mouse_buttons(Game& game, HostService service) -> std::uint8_t;
void opeu_input_reset(Game& game, HostService service);
auto opeu_mouse_wait_click(Game& game, HostService service) -> std::uint8_t;
void opeu_mouse_window(Game& game, HostService service, std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height);

namespace opeu::game::detail::mouse {
// A mouse position in screen pixels.
struct MousePoint {
  std::uint16_t x;
  std::uint16_t y;
};

// The mouse position; 0, 0 without a mouse.
auto MousePosition(Game& game, HostService service) -> MousePoint;
} // namespace opeu::game::detail::mouse

namespace opeu::game {
using detail::mouse::MousePoint;
using detail::mouse::MousePosition;
} // namespace opeu::game

// The module's services: the input driver call the mouse, keyboard and dialog modules share.
auto svc_mouse(Game& game, std::uint16_t call, std::uint16_t* words) -> std::uint16_t;
