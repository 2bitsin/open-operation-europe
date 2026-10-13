// The paratroop regiment: forming it from the troops set aside for the drop, judging landing cells, and landing it.
#include "core/paraunit.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "common/order.hpp"
#include "common/random.hpp"
#include "core/limits.hpp"
#include "types.hpp"

#include <array>
#include <utility>

#include "common/game.hpp"
#include "core/msnrept.hpp"
#include "core/recindex.hpp"
#include "link/glue.hpp"
#include "map/cellflag.hpp"
#include "map/grid.hpp"
#include "map/maperase.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/capacity.hpp"
#include "units/casualty.hpp"
#include "units/elemlink.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {

// Map codes a paratroop regiment cannot land on; an objective cell (OBJECTIVE_CODE) only when the dropping side
// holds it.
constexpr uint8_t OBJECTIVE_CODE{ 1 };
constexpr uint8_t NO_LANDING_CODES[]{ 0, 5, 6, 8, 10 };

constexpr uint8_t ANY_SIDE{ 0xff };
constexpr uint8_t NEIGHBOURS{ 8 };
constexpr uint8_t LANDING_PERCENT{ 90 };
constexpr auto    INTELLIGENCE{ std::to_underlying(StaffRole::INTELLIGENCE_CHIEF) };
constexpr uint8_t STRONG_SIDE{ 50 };

auto CellAt(DataSegment& data, uint8_t x, uint8_t y) -> uint8_t& {
  return data.map_cells[y * MAP_WIDTH + x];
}

// The troops in the unit's battalions.
auto Troops(Game& game, uint16_t unit) -> uint16_t {
  return UnitQuantity(Data(game), unit, ANY_CATEGORY).total;
}

// The unit's capacity for ammunition, fuel and food.
auto Capacities(Game const& game, uint16_t unit) -> std::array<uint16_t, 3> {
  return Figures(UnitCapacities(Data(game), unit));
}

// Steps from (x, y) to the neighbour in direction; 0 when it is off the map.
auto GridStep(uint8_t& x, uint8_t& y, uint8_t direction) -> uint16_t {
  return opeu_grid_step(&x, &y, direction);
}

// The unit takes landing casualties; the cell's byte is kept across them.
void Casualties(Game& game, Unit& unit, uint16_t losses, uint16_t c, uint16_t e) {
  auto&      data{ Data(game) };
  auto const near{ Near(data, unit) };
  auto&      cell{ CellAt(data, unit.x, unit.y) };
  auto const kept{ cell };
  opeu_arrival_prepare(game, near);
  opeu_unit_casualties(game, near, LANDING_PERCENT, losses, c, 0, e);
  cell = kept;
}

} // namespace

// Forms the paratroop regiment: the side's first regiment set aside for the drop is placed at (x, y) with every
// battalion set aside, each with a fresh morale, training and experience, and random ammunition and food bounded by
// its capacities. Returns the regiment's near pointer.
uint16_t opeu_para_unit_create(Game& game, uint8_t side, uint8_t x, uint8_t y) {
  auto&   data{ Data(game) };
  uint8_t slot{ 0 };
  while (slot < UNITS_PER_SIDE && !(data.units[side * UNITS_PER_SIDE + slot].flags & Bits(UnitFlag::AIRBORNE))) ++slot;
  // Without a regiment set aside the one past the side's last is used.
  auto const near{ static_cast<uint16_t>(UNITS_AT + (side * UNITS_PER_SIDE + slot) * sizeof(Unit)) };
  auto&      unit{ Record<Unit>(data, near) };
  if (slot < UNITS_PER_SIDE) {
    unit.first = 0;
    unit.x     = x;
    unit.y     = y;
    unit.moved = 0;
    unit.flags = Bits(UnitFlag::READY) | Bits(UnitFlag::COMMUNICATIONS);
    opeu_clear_unit_orders(game, near, ALL_ORDER_SLOTS);
  }
  auto const kept{ CellAt(data, x, y) };
  for (uint8_t i = 0; i < ELEMENTS_PER_SIDE; ++i) {
    auto& element{ data.elements[side * ELEMENTS_PER_SIDE + i] };
    if (element.flags & Bits(ElementFlag::AIRBORNE)) {
      opeu_insert_element(game, near, Near(data, element));
      element.morale     = 75 - RandomBelow(game, 21);
      element.training   = 60 - RandomBelow(game, 21);
      element.experience = 25 - RandomBelow(game, 21);
      element.flags      = (element.flags & Bits(ElementFlag::SELECTED)) | Bits(ElementFlag::READY);
    }
  }
  CellAt(data, x, y) = kept;
  auto const troops{ Troops(game, near) };
  auto const capacity{ Capacities(game, near) };
  auto const ammunition_roll{ RandomBelow(game, 11) };
  unit.resources[0] = opeu_min16(troops * 5 + ammunition_roll * RandomBelow(game, 21), capacity[0]);
  unit.resources[1] = 0;
  auto const food_roll{ RandomBelow(game, 21) };
  unit.resources[2] = opeu_min16(troops * 5 + food_roll * RandomBelow(game, 21), capacity[2]);
  return near;
}

// 1 when paratroops may land on (x, y): not on the excluded terrain, and on an objective only when the dropping side
// holds it.
uint16_t opeu_para_cell_ok(Game& game, uint8_t x, uint8_t y) {
  auto const code{ opeu_map_code(game, x, y) };
  if (code == OBJECTIVE_CODE) {
    auto const objective{ opeu_find_objective_at(game, x, y) };
    return opeu_objective_side(game, objective) == Data(game).air_side;
  }
  for (auto const excluded : NO_LANDING_CODES)
    if (code == excluded) return 0;
  return 1;
}

// Whether the drop scatters: a scatter value is worked out from the intelligence chief, the weather, the night and the
// side's strength, but it is never used and the drop never scatters.
uint16_t opeu_para_scatter_check(Game& game) {
  auto const& data{ Data(game) };
  auto        value{ opeu_officer_rating(game, data.air_side, INTELLIGENCE) };
  value /= data.weather ? 2 : 1;
  value /= opeu_hour_is_night(data.hour) ? 2 : 1;
  value /= data.sides[data.air_side].strength > STRONG_SIDE ? 1 : 2;
  (void)value;
  return 0;
}

// Lands the paratroop regiment on its cell with `losses` from the flight. On an enemy regiment it fights it; on a
// marked cell it takes heavy losses and claims the cell; on an empty cell it takes light losses and stays. After a
// fight or a claimed cell, a regiment with battalions left moves to a free neighbouring cell it may land on, or is
// stood down when there is none. Returns the regiment's ready bit.
uint16_t opeu_para_land(Game& game, uint16_t near, uint16_t losses) {
  auto&      data{ Data(game) };
  auto&      unit{ Record<Unit>(data, near) };
  auto const ready{ Bits(UnitFlag::READY) };
  auto const side{ opeu_unit_side(near) };
  uint8_t    x{ unit.x };
  uint8_t    y{ unit.y };
  if (opeu_para_scatter_check(game)) {
    GridStep(x, y, static_cast<uint8_t>(RandomBelow(game, 8)));
    if (opeu_para_cell_ok(game, x, y)) {
      unit.x = x;
      unit.y = y;
    } else {
      x = unit.x;
      y = unit.y;
    }
  }
  // Looked for with the regiment itself stood down, so that it does not find itself.
  auto flags{ unit.flags };
  unit.flags &= ~ready;
  auto const other{ opeu_find_unit_at(game, x, y) };
  unit.flags = flags;
  auto const troops{ Troops(game, near) };
  if (other) {
    auto& enemy{ Record<Unit>(data, other) };
    if (opeu_unit_side(other) != side) {
      Casualties(game, unit, troops / 3 + losses, troops / 40, 1);
      if (unit.first) {
        Record<Element>(data, unit.first).flags  |= Bits(ElementFlag::SPOTTED);
        Record<Element>(data, enemy.first).flags |= Bits(ElementFlag::SPOTTED);
      }
    }
    flags       = unit.flags;
    unit.flags &= ~ready;
    opeu_arrival_prepare(game, other);
    unit.flags = flags;
  } else if (opeu_cell_marked(game, x, y, ANY_SIDE)) {
    Casualties(game, unit, troops / 10 + losses, troops / 40, 1);
    opeu_mark_cell(game, x, y, side);
    opeu_map_erase(game, x, y);
  } else {
    Casualties(game, unit, losses >> 1, troops / 100, 0);
    return unit.flags & ready;
  }
  if (!unit.first) return unit.flags & ready;
  for (uint8_t direction = 0; direction < NEIGHBOURS; ++direction) {
    uint8_t to_x{ x };
    uint8_t to_y{ y };
    if (!GridStep(to_x, to_y, direction)) continue;
    if (opeu_cell_side(game, to_x, to_y) != NO_SIDE) continue;
    if (opeu_cell_marked(game, to_x, to_y, ANY_SIDE)) continue;
    if (!opeu_para_cell_ok(game, to_x, to_y)) continue;
    unit.x = to_x;
    unit.y = to_y;
    return unit.flags & ready;
  }
  for (auto& element : ElementChain{ data, unit.first }) element.flags &= ~Bits(ElementFlag::READY);
  unit.flags &= ~ready;
  return unit.flags & ready;
}
