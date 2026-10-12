// Communications on the strategic map: a unit is in communication when it is itself, or when a friendly objective or
// unit in communication stands on or next to its cell. A unit can be selected when the side's selected unit is it,
// or when it, its division's selected unit and the side's selected unit are all in communication.
#include "map/support.hpp"
#include "common/data-segment.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t DIRECTIONS_AND_CENTER{ 9 }; // direction 8 stays on the cell
}

uint16_t opeu_unit_support_condition(const uint8_t* ds, uint16_t unit) {
  auto const& data{ Data(ds) };
  auto const& record{ Record<Unit>(data, unit) };
  if (record.flags & Bits(UnitFlag::COMMUNICATIONS)) return 1;
  auto const side{ opeu_unit_side(unit) };
  for (uint8_t direction{ 0 }; direction < DIRECTIONS_AND_CENTER; ++direction) {
    uint8_t x{ record.x };
    uint8_t y{ record.y };
    if (!opeu_grid_step(&x, &y, direction)) continue;
    auto const objective{ opeu_find_objective_at(ds, x, y) };
    if (objective && opeu_objective_side(ds, objective) == side && (Record<Objective>(data, objective).flags & Bits(ObjectiveFlag::COMMUNICATIONS))) return 1;
    auto const neighbour{ opeu_find_unit_at(ds, x, y) };
    if (neighbour && opeu_unit_side(neighbour) == side && (Record<Unit>(data, neighbour).flags & Bits(UnitFlag::COMMUNICATIONS))) return 1;
  }
  return 0;
}

// A failed selection lookup (0) is tested like a unit at DS:0000.
uint16_t opeu_unit_selection_eligible(const uint8_t* ds, uint16_t unit) {
  auto const division_selected{ opeu_find_group_selected_unit(ds, opeu_unit_group(unit)) };
  auto const side_selected{ opeu_find_side_selected_unit(ds, opeu_unit_side(unit)) };
  if (side_selected == unit) return 1;
  return opeu_unit_support_condition(ds, side_selected) && opeu_unit_support_condition(ds, division_selected) && opeu_unit_support_condition(ds, unit);
}
