// The mouse services (INT 33h).
#pragma once

#include "common/events.hpp"

#include <cstdint>

auto opeu_mouse_reset(std::uint8_t* ds, HostService service) -> std::uint16_t;
void opeu_mouse_show(std::uint8_t* ds, HostService service);
void opeu_mouse_hide(std::uint8_t* ds, HostService service);
void opeu_mouse_position(std::uint8_t* ds, HostService service, std::uint16_t x, std::uint16_t y);
void opeu_mouse_move(std::uint8_t* ds, HostService service, std::uint16_t x, std::uint16_t y);
auto opeu_mouse_left(std::uint8_t* ds, HostService service) -> int;
auto opeu_mouse_right(std::uint8_t* ds, HostService service) -> int;
auto opeu_turn_input_a(std::uint8_t* ds, HostService service) -> int;
auto opeu_mouse_buttons(std::uint8_t* ds, HostService service) -> std::uint8_t;
void opeu_input_reset(std::uint8_t* ds, HostService service);
auto opeu_mouse_wait_click(std::uint8_t* ds, HostService service) -> std::uint8_t;
void opeu_mouse_window(std::uint8_t* ds, HostService service, std::int16_t x, std::int16_t y, std::int16_t width,
                       std::int16_t height);

// The module's services: the input driver call the mouse, keyboard and dialog modules share.
auto svc_mouse(std::uint8_t* ds, std::uint16_t call, std::uint16_t* words) -> std::uint16_t;
