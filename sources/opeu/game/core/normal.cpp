// Puts the turn state in order before a side's turn is processed: units stand on cells their side holds and carry
// no more than they can, the map marks only cells a unit of that side occupies, every active side, division and
// regiment has a selection, and the element chains of fielded regiments are all marked fielded.
#include "core/normal.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "data-segment.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "types.hpp"
#include "units/capacity.hpp"
#include "units/percent.hpp"
#include "units/select.hpp"
#include "units/selelem.hpp"

using namespace opeu::game;

namespace {

// Lowers each supply a fielded unit carries beyond its capacity to the capacity, and marks it again on its cell
// when the cell does not show its side.
void ClampFieldedUnits(Game& game, DataSegment& data) {
  for (auto& unit : data.units) {
    if (!(unit.flags & Bits(UnitFlag::READY))) continue;
    auto const near{ Near(data, unit) };
    if (opeu_cell_side(game, unit.x, unit.y) != opeu_unit_side(near)) opeu_place_unit(game, near);
    uint16_t percent[3];
    uint16_t capacity[3];
    opeu_unit_resource_percentages(game, near, &percent[0], &percent[1], &percent[2]);
    opeu_unit_capacities(game, near, &capacity[0], &capacity[1], &capacity[2]);
    for (auto i = 0; i < 3; ++i)
      if (unit.resources[i] && percent[i] > 100) unit.resources[i] = capacity[i];
  }
}

// Clears the occupant bits of every cell whose marked side has no unit of its own there.
void ClearStaleOccupants(Game& game, DataSegment& data) {
  for (uint8_t y = 0; y < MAP_HEIGHT; ++y)
    for (uint8_t x = 0; x < MAP_WIDTH; ++x) {
      auto const side{ opeu_cell_side(game, x, y) };
      if (side == 0xff) continue;
      auto const unit{ opeu_find_unit_at(game, x, y) };
      if (!unit || side != opeu_unit_side(unit)) MapCellAt(data, x, y) &= ~Bits(MapCell::OCCUPANT_MASK);
    }
}

// Gives every side a selected division, every active division a selected regiment and every fielded regiment a
// selected element.
void FillSelections(Game& game, DataSegment& data) {
  for (uint8_t side = 0; side < SIDE_COUNT; ++side) {
    if (!opeu_find_side_selected_unit(game, side)) opeu_select_first_group(game, side);
    for (uint8_t division = 0; division < GROUPS_PER_SIDE; ++division) {
      auto const& group{ GroupOf(data, side, division) };
      auto const  group_near{ Near(data, group) };
      if ((group.flags & Bits(GroupFlag::READY)) && !opeu_find_group_selected_unit(game, group_near)) opeu_select_first_unit(game, group_near);
      for (uint8_t i = 0; i < UNITS_PER_GROUP; ++i) {
        auto const& unit{ data.units[(side * GROUPS_PER_SIDE + division) * UNITS_PER_GROUP + i] };
        auto const  unit_near{ Near(data, unit) };
        if ((unit.flags & Bits(UnitFlag::READY)) && !opeu_unit_selected_element(game, unit_near)) opeu_select_preferred_element(game, unit_near);
      }
    }
  }
}

// Stands down fielded regiments without elements, and active divisions without a fielded regiment that has any.
void StandDownEmpty(DataSegment& data) {
  for (uint8_t side = 0; side < SIDE_COUNT; ++side)
    for (uint8_t division = 0; division < GROUPS_PER_SIDE; ++division) {
      auto& group{ GroupOf(data, side, division) };
      auto  manned{ false };
      for (uint8_t i = 0; i < UNITS_PER_GROUP; ++i) {
        auto& unit{ data.units[(side * GROUPS_PER_SIDE + division) * UNITS_PER_GROUP + i] };
        if (!(unit.flags & Bits(UnitFlag::READY))) continue;
        if (!unit.first)
          unit.flags &= ~Bits(UnitFlag::READY);
        else
          manned = true;
      }
      if ((group.flags & Bits(GroupFlag::READY)) && !manned) group.flags &= ~Bits(GroupFlag::READY);
    }
}

} // namespace

void opeu_normalize_turn_state(Game& game) {
  auto& data{ Data(game) };
  ClampFieldedUnits(game, data);
  ClearStaleOccupants(game, data);
  FillSelections(game, data);
  StandDownEmpty(data);
  for (auto const& unit : data.units)
    if (unit.flags & Bits(UnitFlag::READY))
      for (auto& element : Elements(data, unit)) element.flags |= Bits(ElementFlag::READY);
}
