// Communications on the strategic map: a unit is in communication when it is itself, or when a friendly objective or
// unit in communication stands on or next to its cell. A unit can be selected when the side's selected unit is it,
// or when it, its division's selected unit and the side's selected unit are all in communication.
#include "map/support.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include <algorithm>
#include <ranges>

using namespace opeu::game;

uint16_t opeu_unit_support_condition(Game const& game, uint16_t unit) {
  auto const& data{ Data(game) };
  auto const& record{ Record<Unit>(data, unit) };
  if (record.flags & Bits(UnitFlag::COMMUNICATIONS)) return 1;
  auto const side{ opeu_unit_side(unit) };
  auto const in_communication{ [&](uint8_t direction) {
    auto const cell{ StepOnMap({ record.x, record.y }, direction) };
    if (!cell) return false;
    auto const objective{ opeu_find_objective_at(game, cell->x, cell->y) };
    if (objective && opeu_objective_side(game, objective) == side && (Record<Objective>(data, objective).flags & Bits(ObjectiveFlag::COMMUNICATIONS))) return true;
    auto const neighbour{ opeu_find_unit_at(game, cell->x, cell->y) };
    return neighbour && opeu_unit_side(neighbour) == side && (Record<Unit>(data, neighbour).flags & Bits(UnitFlag::COMMUNICATIONS));
  } };
  return std::ranges::any_of(std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS_AND_CENTER), in_communication) ? 1 : 0;
}

// A failed selection lookup (0) is tested like a unit at DS:0000.
uint16_t opeu_unit_selection_eligible(Game const& game, uint16_t unit) {
  auto const division_selected{ opeu_find_group_selected_unit(game, opeu_unit_group(unit)) };
  auto const side_selected{ opeu_find_side_selected_unit(game, opeu_unit_side(unit)) };
  if (side_selected == unit) return 1;
  return opeu_unit_support_condition(game, side_selected) && opeu_unit_support_condition(game, division_selected) && opeu_unit_support_condition(game, unit);
}
