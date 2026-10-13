// Button menus: drawing and mouse selection.
#pragma once

#include "common/events.hpp"

#include <cstdint>

void opeu_menu_text(Game& game, std::uint16_t x, std::uint16_t y, std::uint16_t attribute, std::uint16_t label);
void opeu_menu_draw_item(Game& game, std::uint16_t item, int lit);
void opeu_menu_draw(Game& game, std::uint16_t item);
void opeu_menu_restore(Game& game, std::uint16_t item);
auto opeu_menu_track(Game& game, std::uint16_t items, int cancel) -> std::uint16_t;
auto opeu_menu_select(Game& game, std::uint16_t items, int cancel) -> std::uint16_t;

// The module's services: the shared input and graphics calls.
auto svc_menudraw(Game& game, std::uint16_t call, std::uint16_t* words) -> std::uint16_t;
