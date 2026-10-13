// A step of a map cell that is taken only when it stays on the map.
#include "map/stepmap.hpp"
#include "common/map-grid.hpp"
#include "map/step.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

using namespace opeu::game;

uint16_t opeu_grid_step(uint8_t* x, uint8_t* y, uint8_t direction) {
  uint8_t next_x{ *x };
  uint8_t next_y{ *y };
  opeu_grid_step_unchecked(&next_x, &next_y, direction);
  if (!OnMap(next_x, next_y)) return 0;
  *x = next_x;
  *y = next_y;
  return 1;
}
