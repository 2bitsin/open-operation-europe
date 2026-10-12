// Keyboard waits, the text cursor, decimal entry, value choice and yes/no prompts.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_text_cursor_clear(std::uint8_t* ds, HostService service);
void opeu_text_cursor_draw(std::uint8_t* ds, HostService service);
auto opeu_key_wait_tick(std::uint8_t* ds, HostService service) -> int;
auto opeu_keyboard_key(std::uint8_t* ds, HostService service) -> std::uint8_t;
auto opeu_mouse_wait_buttons(std::uint8_t* ds, HostService service) -> std::uint8_t;
void opeu_wait_confirm(std::uint8_t* ds, HostService service, int pause);
auto opeu_read_decimal(std::uint8_t* ds, HostService service, std::uint16_t digits, std::uint16_t scratch)
    -> std::int32_t;
auto opeu_choose_value(std::uint8_t* ds, HostService service, std::uint16_t min, std::uint16_t max, int cancel,
                       std::uint16_t items, std::uint16_t scratch) -> std::uint16_t;
auto opeu_prompt_value(std::uint8_t* ds, HostService service, std::uint16_t text, std::uint16_t min, std::uint16_t max,
                       int cancel, std::uint16_t items, std::uint16_t scratch) -> std::uint16_t;
auto opeu_prompt_value_cancel(std::uint8_t* ds, HostService service, std::uint16_t text, std::uint16_t max,
                              std::uint16_t items, std::uint16_t scratch) -> std::uint16_t;
auto opeu_prompt_value_required(std::uint8_t* ds, HostService service, std::uint16_t text, std::uint16_t max,
                                std::uint16_t items, std::uint16_t scratch) -> std::uint16_t;
auto opeu_mouse_yes_no(std::uint8_t* ds, HostService service, std::uint16_t scratch) -> std::uint8_t;
auto opeu_yes_no(std::uint8_t* ds, HostService service, std::uint16_t scratch) -> int;
auto opeu_prompt_yes_no(std::uint8_t* ds, HostService service, std::uint16_t text, std::uint16_t scratch) -> int;

// The module's services: the shared input calls of the mouse, menus, numeric dialog and keyboard.
auto svc_keyboard(std::uint8_t* ds, std::uint16_t call, std::uint16_t* w) -> std::uint16_t;
