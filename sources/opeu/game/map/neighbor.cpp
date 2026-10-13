// Units that can take supplies from a neighbour: a side's units next to a friendly objective or a friendly unit of the
// same class, and a unit's same-class friendly neighbours. Both write the units' near pointers as words to the DS
// buffer at out and return how many they wrote.
#include "map/neighbor.hpp"
#include "common/data-segment.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/recindex.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/typetext.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t CENTER{ 8 }; // the direction that stays on the cell

void Store(uint8_t* ds, uint16_t& out, uint16_t unit) {
  SetWord(ds, out, unit);
  out = static_cast<uint16_t>(out + 2);
}
} // namespace

// The side's ready, selectable units with a friendly objective on or next to their cell, or a friendly unit of the
// same class next to it. The friendly side is the current side.
uint8_t opeu_side_supported_units(uint8_t* ds, uint8_t side, uint16_t out) {
  auto&   data{ Data(ds) };
  uint8_t found{ 0 };
  for (uint8_t i{ 0 }; i < UNITS_PER_SIDE; ++i) {
    auto const& record{ data.units[side * UNITS_PER_SIDE + i] };
    auto const  unit{ Near(data, record) };
    if (!(record.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(ds, unit)) continue;
    for (uint8_t direction{ 0 }; direction <= CENTER; ++direction) {
      uint8_t x{ record.x };
      uint8_t y{ record.y };
      if (!opeu_grid_step(&x, &y, direction)) continue;
      auto const objective{ opeu_find_objective_at(ds, x, y) };
      bool       supported{ objective != 0 && opeu_objective_side(ds, objective) == data.current_side };
      if (!supported && direction != CENTER) {
        auto const neighbour{ opeu_find_unit_at(ds, x, y) };
        supported = neighbour != 0 && opeu_unit_side(neighbour) == data.current_side && opeu_same_unit_class(unit, neighbour);
      }
      if (supported) {
        Store(ds, out, unit);
        ++found;
        break;
      }
    }
  }
  return found;
}

// The selectable units of the unit's side next to it that are of its class.
uint8_t opeu_neighbor_units(uint8_t* ds, uint16_t unit, uint16_t out) {
  auto const& record{ Record<Unit>(Data(ds), unit) };
  auto const  side{ opeu_unit_side(unit) };
  uint8_t     found{ 0 };
  for (uint8_t direction{ 0 }; direction < GRID_DIRECTIONS; ++direction) {
    uint8_t x{ record.x };
    uint8_t y{ record.y };
    if (!opeu_grid_step(&x, &y, direction)) continue;
    auto const neighbour{ opeu_find_unit_at(ds, x, y) };
    if (neighbour != 0 && opeu_unit_side(neighbour) == side && opeu_unit_selection_eligible(ds, neighbour) && opeu_same_unit_class(unit, neighbour)) {
      Store(ds, out, neighbour);
      ++found;
    }
  }
  return found;
}
