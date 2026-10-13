// Pans the tactical view by a step: the cursor cell is erased first, then the view scrolls to its origin plus the step.
#include "map/viewpan.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "map/maperase.hpp"
#include "map/mapscrol.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

void opeu_viewport_pan(Game& game, ViewPan const& pan) {
  auto const& data{ Data(game) };
  opeu_map_erase(game, pan.cursor.x, pan.cursor.y);
  opeu_map_scroll_to(game, static_cast<int16_t>(static_cast<int16_t>(data.viewport_x) + pan.dx), static_cast<int16_t>(static_cast<int16_t>(data.viewport_y) + pan.dy), pan.side);
}
