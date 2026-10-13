// Keyboard waits, the text cursor, decimal entry, value choice and yes/no prompts.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_text_cursor_clear(Game& game, HostService service);
void opeu_text_cursor_draw(Game& game);
auto opeu_key_wait_tick(Game& game, HostService service) -> int;
auto opeu_keyboard_key(Game& game, HostService service) -> std::uint8_t;
auto opeu_mouse_wait_buttons(Game& game, HostService service) -> std::uint8_t;
void opeu_wait_confirm(Game& game, HostService service, int pause);
auto opeu_read_decimal(Game& game, HostService service, std::uint16_t digits) -> std::int32_t;
auto opeu_choose_value(Game& game, HostService service, std::uint16_t min, std::uint16_t max, int cancel, std::uint16_t items) -> std::uint16_t;
auto opeu_prompt_value(Game& game, HostService service, std::uint16_t text, std::uint16_t min, std::uint16_t max, int cancel, std::uint16_t items) -> std::uint16_t;
auto opeu_prompt_value_cancel(Game& game, HostService service, std::uint16_t text, std::uint16_t max, std::uint16_t items) -> std::uint16_t;
auto opeu_prompt_value_required(Game& game, HostService service, std::uint16_t text, std::uint16_t max, std::uint16_t items) -> std::uint16_t;
auto opeu_mouse_yes_no(Game& game, HostService service) -> std::uint8_t;
auto opeu_yes_no(Game& game, HostService service) -> int;
auto opeu_prompt_yes_no(Game& game, HostService service, std::uint16_t text) -> int;

// The module's services: the shared input calls of the mouse, menus, numeric dialog and keyboard.
auto svc_keyboard(Game& game, std::uint16_t call, std::uint16_t* w) -> std::uint16_t;
