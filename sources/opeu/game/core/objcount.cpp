// Counting the objectives a side holds.
#include "core/objcount.hpp"
#include "data-segment.hpp"
#include "map/query.hpp"
#include "types.hpp"

using namespace opeu::game;

uint8_t opeu_count_active_objectives(const uint8_t ds[65536], uint8_t side) {
  auto const& data{ Data(ds) };
  uint8_t     count{ 0 };
  for (auto const& objective : data.objectives)
    if (objective.active && opeu_objective_side(ds, Near(data, objective)) == side) ++count;
  return count;
}
