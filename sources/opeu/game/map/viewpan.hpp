// A pan of the tactical view by a step (map/viewpan.cpp).
#pragma once

#include "common/events.hpp"
#include "common/grid-cell.hpp"
#include "types.hpp"

namespace opeu::game::detail::viewpan {

// A pan: the view's origin moves by (dx, dy) cells, the cursor cell is erased first, and the side's view is redrawn.
struct ViewPan {
  std::int16_t dx;
  std::int16_t dy;
  GridCell     cursor;
  std::uint8_t side;
};
} // namespace opeu::game::detail::viewpan

namespace opeu::game {
using detail::viewpan::ViewPan;
} // namespace opeu::game

void opeu_viewport_pan(Game& game, opeu::game::ViewPan const& pan);
