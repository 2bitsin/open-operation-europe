#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

#include <cstdint>

namespace opeu::game::detail::btlelem {

// An element as drawn on the field: its near pointer and its mode (the facing it shows).
struct ShownElement {
  std::uint16_t element;
  std::uint8_t  mode;
};

// An animation over a cell: its first frame in the icon segment, the element drawn under it (0: none), and whether
// the animation handler runs after the first frame (pause), with the turn service and the handler again (turn).
struct CellAnimation {
  std::uint16_t frames;
  std::uint16_t element;
  bool          pause;
  bool          turn;
};
} // namespace opeu::game::detail::btlelem

namespace opeu::game {
using detail::btlelem::CellAnimation;
using detail::btlelem::ShownElement;
} // namespace opeu::game

// Places the element on the cell and draws it there when it is visible.
void opeu_battle_draw_element_at(Game& game, opeu::game::GridCell cell, opeu::game::ShownElement shown);
// Redraws the cell with the element on it (0: the bare cell).
void opeu_battle_redraw_cell(Game& game, opeu::game::GridCell cell, opeu::game::ShownElement shown);
void opeu_battle_refresh_cell(Game& game, uint8_t x, uint8_t y);
void opeu_battle_animate_element(Game& game, opeu::game::GridCell cell, opeu::game::CellAnimation const& animation);
