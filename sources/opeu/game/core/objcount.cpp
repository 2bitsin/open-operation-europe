// Counting the objectives a side holds.
#include "core/objcount.hpp"
#include "common/game.hpp"
#include "data-segment.hpp"
#include "map/query.hpp"
#include "types.hpp"

using namespace opeu::game;

uint8_t opeu_count_active_objectives(Game const& game, uint8_t side) {
  auto const& data{ Data(game) };
  uint8_t     count{ 0 };
  for (auto const& objective : data.objectives)
    if (objective.active && opeu_objective_side(game, Near(data, objective)) == side) ++count;
  return count;
}
