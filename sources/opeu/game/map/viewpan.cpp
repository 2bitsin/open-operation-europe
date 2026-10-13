// Pans the tactical view by a step: the cursor cell is erased first, then the view scrolls to its origin plus the step.
#include "map/viewpan.hpp"
#include "common/data-segment.hpp"
#include "common/map-frame.hpp"
#include "common/memory.hpp"
#include "map/maperase.hpp"
#include "map/mapscrol.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

namespace { } // namespace

void opeu_viewport_pan(Game& game, int16_t dx, int16_t dy, uint8_t x, uint8_t y, uint8_t side) {
  auto const& data{ Data(game) };
  opeu_map_erase(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
  opeu_map_scroll_to(game, static_cast<int16_t>(static_cast<int16_t>(data.viewport_x) + dx), static_cast<int16_t>(static_cast<int16_t>(data.viewport_y) + dy), static_cast<uint8_t>(side));
}
