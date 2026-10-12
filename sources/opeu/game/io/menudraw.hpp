// Button menus: drawing and mouse selection.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_menu_text(std::uint8_t* ds, HostService service, std::uint16_t x, std::uint16_t y, std::uint16_t attribute, std::uint16_t label);
void opeu_menu_draw_item(std::uint8_t* ds, HostService service, std::uint16_t item, int lit);
void opeu_menu_draw(std::uint8_t* ds, HostService service, std::uint16_t item);
void opeu_menu_restore(std::uint8_t* ds, HostService service, std::uint16_t item);
auto opeu_menu_track(std::uint8_t* ds, HostService service, std::uint16_t items, int cancel, std::uint16_t scratch) -> std::uint16_t;
auto opeu_menu_select(std::uint8_t* ds, HostService service, std::uint16_t items, int cancel, std::uint16_t scratch) -> std::uint16_t;

// The module's services: the shared input and graphics calls.
auto svc_menudraw(std::uint8_t* ds, std::uint16_t call, std::uint16_t* words) -> std::uint16_t;
