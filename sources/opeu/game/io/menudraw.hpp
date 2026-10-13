// Button menus: drawing and mouse selection.
#pragma once

#include "common/events.hpp"

#include <cstdint>

namespace opeu::game::detail::menudraw {
// How a button is drawn.
enum class MenuLight : std::uint8_t {
  PLAIN,
  LIT,
};
} // namespace opeu::game::detail::menudraw

namespace opeu::game {
using detail::menudraw::MenuLight;
} // namespace opeu::game

// Items are near pointers to MenuItem records; a menu runs from its first item up to the terminator.
void opeu_menu_draw_item(Game& game, std::uint16_t item, opeu::game::MenuLight light);
void opeu_menu_draw(Game& game, std::uint16_t first);
void opeu_menu_restore(Game& game, std::uint16_t first);
auto opeu_menu_select(Game& game, std::uint16_t first, int cancel) -> std::uint16_t;
