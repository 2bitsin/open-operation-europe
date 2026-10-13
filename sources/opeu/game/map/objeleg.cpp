// Whether an objective can be supplied: it must be in communication itself or have a friendly unit in communication on
// or next to it, and the side's selected unit must be in communication.
#include "map/objeleg.hpp"
#include "common/data-segment.hpp"
#include "common/map-grid.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

#include <algorithm>
#include <ranges>

using namespace opeu::game;

namespace {

[[nodiscard]] auto NeighbourInCommunication(Game const& game, uint16_t objective) -> bool {
  auto const& data{ Data(game) };
  auto const& record{ Record<Objective>(data, objective) };
  if (record.flags & Bits(ObjectiveFlag::COMMUNICATIONS)) return true;
  auto const in_communication{ [&](uint8_t direction) {
    auto const cell{ StepOnMap({ record.x, record.y }, direction) };
    if (!cell) return false;
    auto const unit{ opeu_find_unit_at(game, cell->x, cell->y) };
    return unit && opeu_unit_side(unit) == opeu_objective_side(game, objective) && (Record<Unit>(data, unit).flags & Bits(UnitFlag::COMMUNICATIONS));
  } };
  return std::ranges::any_of(std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS_AND_CENTER), in_communication);
}
} // namespace

uint16_t opeu_objective_eligible(Game const& game, uint16_t objective) {
  if (!NeighbourInCommunication(game, objective)) return 0;
  return opeu_unit_support_condition(game, opeu_find_side_selected_unit(game, opeu_objective_side(game, objective)));
}
