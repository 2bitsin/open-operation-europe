// Units that can take supplies from a neighbour: a side's units next to a friendly objective or a friendly unit of the
// same class, and a unit's same-class friendly neighbours. Both write the units' near pointers as words to the DS
// buffer at out and return how many they wrote.
#include "map/neighbor.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/recindex.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/typetext.hpp"

#include <ranges>

using namespace opeu::game;

namespace {
constexpr uint8_t CENTER{ 8 }; // the direction that stays on the cell

void Store(Game& game, uint16_t& out, uint16_t unit) {
  SetWord(game, out, unit);
  out = static_cast<uint16_t>(out + 2);
}
} // namespace

// The side's ready, selectable units with a friendly objective on or next to their cell, or a friendly unit of the
// same class next to it. The friendly side is the current side.
uint8_t opeu_side_supported_units(Game& game, uint8_t side, uint16_t out) {
  auto&   data{ Data(game) };
  uint8_t found{ 0 };
  for (auto const i : std::views::iota(uint8_t{ 0 }, UNITS_PER_SIDE)) {
    auto const& record{ data.units[side * UNITS_PER_SIDE + i] };
    auto const  unit{ Near(data, record) };
    if (!(record.flags & Bits(UnitFlag::READY)) || !opeu_unit_selection_eligible(game, unit)) continue;
    for (auto const direction : std::views::iota(uint8_t{ 0 }, static_cast<uint8_t>(CENTER + 1))) {
      auto const cell{ StepOnMap({ record.x, record.y }, direction) };
      if (!cell) continue;
      auto const objective{ opeu_find_objective_at(game, cell->x, cell->y) };
      bool       supported{ objective != 0 && opeu_objective_side(game, objective) == data.current_side };
      if (!supported && direction != CENTER) {
        auto const neighbour{ opeu_find_unit_at(game, cell->x, cell->y) };
        supported = neighbour != 0 && opeu_unit_side(neighbour) == data.current_side && opeu_same_unit_class(unit, neighbour);
      }
      if (supported) {
        Store(game, out, unit);
        ++found;
        break;
      }
    }
  }
  return found;
}

// The selectable units of the unit's side next to it that are of its class.
uint8_t opeu_neighbor_units(Game& game, uint16_t unit, uint16_t out) {
  auto const& record{ Record<Unit>(Data(game), unit) };
  auto const  side{ opeu_unit_side(unit) };
  uint8_t     found{ 0 };
  for (auto const direction : std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS)) {
    auto const cell{ StepOnMap({ record.x, record.y }, direction) };
    if (!cell) continue;
    auto const neighbour{ opeu_find_unit_at(game, cell->x, cell->y) };
    if (neighbour != 0 && opeu_unit_side(neighbour) == side && opeu_unit_selection_eligible(game, neighbour) && opeu_same_unit_class(unit, neighbour)) {
      Store(game, out, neighbour);
      ++found;
    }
  }
  return found;
}
