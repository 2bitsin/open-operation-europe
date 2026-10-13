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

// The services the paratroop routines call through their event. QUANTITY, CAPACITIES and GRID_STEP write their
// results back into the argument array.
enum class ParaService : uint16_t {
  CLEAR_ORDERS    = 0,  // (unit, slots)
  INSERT_ELEMENT  = 1,  // (unit, element)
  QUANTITY        = 2,  // (unit, category, count) -> args[2] the count
  CAPACITIES      = 3,  // (unit, a, b, c) -> args[1..3]
  MAP_CODE        = 4,  // (x, y): the terrain code of the cell
  OBJECTIVE_AT    = 5,  // (x, y)
  OBJECTIVE_SIDE  = 6,  // (objective)
  OFFICER_RATING  = 7,  // (side, kind)
  IS_NIGHT        = 8,  // (hour)
  UNIT_SIDE       = 9,  // (unit)
  GRID_STEP       = 10, // (x, y, direction) -> args[0..1] the neighbour, 0 off the map
  UNIT_AT         = 11, // (x, y)
  ARRIVAL_PREPARE = 12, // (unit)
  CASUALTIES      = 13, // (unit, percent, losses, c, d, e)
  CELL_MARKED     = 14, // (x, y, side)
  MARK_CELL       = 15, // (x, y, side)
  MAP_ERASE       = 16, // (x, y)
  CELL_SIDE       = 17, // (x, y)
};

// Map codes a paratroop regiment cannot land on; an objective cell (OBJECTIVE_CODE) only when the dropping side
// holds it.
constexpr uint8_t OBJECTIVE_CODE{ 1 };
constexpr uint8_t NO_LANDING_CODES[]{ 0, 5, 6, 8, 10 };

constexpr uint8_t ALL_CATEGORIES{ 0xff };
constexpr uint8_t ANY_SIDE{ 0xff };
constexpr uint8_t NEIGHBOURS{ 8 };
constexpr uint8_t LANDING_PERCENT{ 90 };
constexpr auto    INTELLIGENCE{ std::to_underlying(StaffRole::INTELLIGENCE_CHIEF) };
constexpr uint8_t STRONG_SIDE{ 50 };

auto CellAt(DataSegment& data, uint8_t x, uint8_t y) -> uint8_t& {
  return data.map_cells[y * MAP_WIDTH + x];
}

// The troops in the unit's battalions.
auto Troops(Game& game, HostWordsEvent event, uint16_t unit) -> uint16_t {
  uint16_t args[]{ unit, ALL_CATEGORIES, 0 };
  return event(game, std::to_underlying(ParaService::QUANTITY), args, 3);
}

// The unit's capacity for ammunition, fuel and food.
auto Capacities(Game& game, HostWordsEvent event, uint16_t unit) -> std::array<uint16_t, 3> {
  uint16_t args[]{ unit, 0, 0, 0 };
  event(game, std::to_underlying(ParaService::CAPACITIES), args, 4);
  return { args[1], args[2], args[3] };
}

// Steps from (x, y) to the neighbour in direction; 0 when it is off the map.
auto GridStep(Game& game, HostWordsEvent event, uint8_t& x, uint8_t& y, uint8_t direction) -> uint16_t {
  uint16_t   args[]{ x, y, direction };
  auto const result{ event(game, std::to_underlying(ParaService::GRID_STEP), args, 3) };
  x = static_cast<uint8_t>(args[0]);
  y = static_cast<uint8_t>(args[1]);
  return result;
}

// The unit takes landing casualties; the cell's byte is kept across them.
void Casualties(Game& game, HostWordsEvent event, Unit& unit, uint16_t losses, uint16_t c, uint16_t e) {
  auto&      data{ Data(game) };
  auto const near{ Near(data, unit) };
  auto&      cell{ CellAt(data, unit.x, unit.y) };
  auto const kept{ cell };
  Call(game, event, ParaService::ARRIVAL_PREPARE, near);
  Call(game, event, ParaService::CASUALTIES, near, LANDING_PERCENT, losses, c, 0, e);
  cell = kept;
}

} // namespace

// Forms the paratroop regiment: the side's first regiment set aside for the drop is placed at (x, y) with every
// battalion set aside, each with a fresh morale, training and experience, and random ammunition and food bounded by
// its capacities. Returns the regiment's near pointer.
uint16_t opeu_para_unit_create(Game& game, HostWordsEvent event, uint8_t side, uint8_t x, uint8_t y) {
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
    Call(game, event, ParaService::CLEAR_ORDERS, near, ALL_ORDER_SLOTS, 0);
  }
  auto const kept{ CellAt(data, x, y) };
  for (uint8_t i = 0; i < ELEMENTS_PER_SIDE; ++i) {
    auto& element{ data.elements[side * ELEMENTS_PER_SIDE + i] };
    if (element.flags & Bits(ElementFlag::AIRBORNE)) {
      Call(game, event, ParaService::INSERT_ELEMENT, near, Near(data, element), 0);
      element.morale     = 75 - RandomBelow(game, 21);
      element.training   = 60 - RandomBelow(game, 21);
      element.experience = 25 - RandomBelow(game, 21);
      element.flags      = (element.flags & Bits(ElementFlag::SELECTED)) | Bits(ElementFlag::READY);
    }
  }
  CellAt(data, x, y) = kept;
  auto const troops{ Troops(game, event, near) };
  auto const capacity{ Capacities(game, event, near) };
  auto const ammunition_roll{ RandomBelow(game, 11) };
  unit.resources[0] = opeu_min16(troops * 5 + ammunition_roll * RandomBelow(game, 21), capacity[0]);
  unit.resources[1] = 0;
  auto const food_roll{ RandomBelow(game, 21) };
  unit.resources[2] = opeu_min16(troops * 5 + food_roll * RandomBelow(game, 21), capacity[2]);
  return near;
}

// 1 when paratroops may land on (x, y): not on the excluded terrain, and on an objective only when the dropping side
// holds it.
uint16_t opeu_para_cell_ok(Game& game, HostWordsEvent event, uint8_t x, uint8_t y) {
  auto const code{ static_cast<uint8_t>(Call(game, event, ParaService::MAP_CODE, x, y, 0)) };
  if (code == OBJECTIVE_CODE) {
    auto const objective{ Call(game, event, ParaService::OBJECTIVE_AT, x, y, 0) };
    return static_cast<uint8_t>(Call(game, event, ParaService::OBJECTIVE_SIDE, objective, 0, 0)) == Data(game).air_side;
  }
  for (auto const excluded : NO_LANDING_CODES)
    if (code == excluded) return 0;
  return 1;
}

// Whether the drop scatters: a scatter value is worked out from the intelligence chief, the weather, the night and the
// side's strength, but it is never used and the drop never scatters.
uint16_t opeu_para_scatter_check(Game& game, HostWordsEvent event) {
  auto const& data{ Data(game) };
  auto        value{ Call(game, event, ParaService::OFFICER_RATING, data.air_side, INTELLIGENCE, 0) };
  value /= data.weather ? 2 : 1;
  value /= Call(game, event, ParaService::IS_NIGHT, data.hour, 0, 0) ? 2 : 1;
  value /= data.sides[data.air_side].strength > STRONG_SIDE ? 1 : 2;
  (void)value;
  return 0;
}

// Lands the paratroop regiment on its cell with `losses` from the flight. On an enemy regiment it fights it; on a
// marked cell it takes heavy losses and claims the cell; on an empty cell it takes light losses and stays. After a
// fight or a claimed cell, a regiment with battalions left moves to a free neighbouring cell it may land on, or is
// stood down when there is none. Returns the regiment's ready bit.
uint16_t opeu_para_land(Game& game, HostWordsEvent event, uint16_t near, uint16_t losses) {
  auto&      data{ Data(game) };
  auto&      unit{ Record<Unit>(data, near) };
  auto const ready{ Bits(UnitFlag::READY) };
  auto const side{ static_cast<uint8_t>(Call(game, event, ParaService::UNIT_SIDE, near, 0, 0)) };
  uint8_t    x{ unit.x };
  uint8_t    y{ unit.y };
  if (opeu_para_scatter_check(game, event)) {
    GridStep(game, event, x, y, static_cast<uint8_t>(RandomBelow(game, 8)));
    if (opeu_para_cell_ok(game, event, x, y)) {
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
  auto const other{ Call(game, event, ParaService::UNIT_AT, x, y, 0) };
  unit.flags = flags;
  auto const troops{ Troops(game, event, near) };
  if (other) {
    auto& enemy{ Record<Unit>(data, other) };
    if (static_cast<uint8_t>(Call(game, event, ParaService::UNIT_SIDE, other, 0, 0)) != side) {
      Casualties(game, event, unit, troops / 3 + losses, troops / 40, 1);
      if (unit.first) {
        Record<Element>(data, unit.first).flags  |= Bits(ElementFlag::SPOTTED);
        Record<Element>(data, enemy.first).flags |= Bits(ElementFlag::SPOTTED);
      }
    }
    flags       = unit.flags;
    unit.flags &= ~ready;
    Call(game, event, ParaService::ARRIVAL_PREPARE, other);
    unit.flags = flags;
  } else if (Call(game, event, ParaService::CELL_MARKED, x, y, ANY_SIDE)) {
    Casualties(game, event, unit, troops / 10 + losses, troops / 40, 1);
    Call(game, event, ParaService::MARK_CELL, x, y, side);
    Call(game, event, ParaService::MAP_ERASE, x, y, 0);
  } else {
    Casualties(game, event, unit, losses >> 1, troops / 100, 0);
    return unit.flags & ready;
  }
  if (!unit.first) return unit.flags & ready;
  for (uint8_t direction = 0; direction < NEIGHBOURS; ++direction) {
    uint8_t to_x{ x };
    uint8_t to_y{ y };
    if (!GridStep(game, event, to_x, to_y, direction)) continue;
    if (static_cast<uint8_t>(Call(game, event, ParaService::CELL_SIDE, to_x, to_y, 0)) != NO_SIDE) continue;
    if (Call(game, event, ParaService::CELL_MARKED, to_x, to_y, ANY_SIDE)) continue;
    if (!opeu_para_cell_ok(game, event, to_x, to_y)) continue;
    unit.x = to_x;
    unit.y = to_y;
    return unit.flags & ready;
  }
  for (auto& element : ElementChain{ data, unit.first }) element.flags &= ~Bits(ElementFlag::READY);
  unit.flags &= ~ready;
  return unit.flags & ready;
}

uint16_t svc_paraunit(Game& game, uint16_t id, uint16_t* args, uint16_t count) {
  uint16_t r{ 0 };
  switch (static_cast<ParaService>(id)) {
  case ParaService::CLEAR_ORDERS:   opeu_clear_unit_orders(game, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case ParaService::INSERT_ELEMENT: opeu_insert_element(game, Arg(args, count, 0), Arg(args, count, 1)); break;
  case ParaService::QUANTITY:       {
    uint8_t n = (uint8_t)Arg(args, count, 2);
    r         = opeu_unit_quantity(game, Arg(args, count, 0), (uint8_t)Arg(args, count, 1), &n);
    if (count > 2) args[2] = n;
    break;
  }
  case ParaService::CAPACITIES: {
    uint16_t a = Arg(args, count, 1), b = Arg(args, count, 2), c = Arg(args, count, 3);
    opeu_unit_capacities(game, Arg(args, count, 0), &a, &b, &c);
    if (count > 3) {
      args[1] = a;
      args[2] = b;
      args[3] = c;
    }
    break;
  }
  case ParaService::MAP_CODE:       r = opeu_map_code(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case ParaService::OBJECTIVE_AT:   r = opeu_find_objective_at(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case ParaService::OBJECTIVE_SIDE: r = opeu_objective_side(game, Arg(args, count, 0)); break;
  case ParaService::OFFICER_RATING: r = opeu_officer_rating(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case ParaService::IS_NIGHT:       r = opeu_hour_is_night((uint8_t)Arg(args, count, 0)); break;
  case ParaService::UNIT_SIDE:      r = opeu_unit_side(Arg(args, count, 0)); break;
  case ParaService::GRID_STEP:      {
    uint8_t x = (uint8_t)Arg(args, count, 0), y = (uint8_t)Arg(args, count, 1);
    r = opeu_grid_step(&x, &y, (uint8_t)Arg(args, count, 2));
    if (count > 1) {
      args[0] = x;
      args[1] = y;
    }
    break;
  }
  case ParaService::UNIT_AT:         r = opeu_find_unit_at(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case ParaService::ARRIVAL_PREPARE: {
    opeu_arrival_prepare(game, Arg(args, count, 0));
    break;
  }
  case ParaService::CASUALTIES: {
    opeu_unit_casualties(game, svc_casualty, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5));
    break;
  }
  case ParaService::CELL_MARKED: r = opeu_cell_marked(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case ParaService::MARK_CELL:   opeu_mark_cell(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case ParaService::MAP_ERASE:   {
    opeu_map_erase(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
    break;
  }
  case ParaService::CELL_SIDE: r = opeu_cell_side(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  default:                     port_unhandled("paraunit", id, count);
  }
  return r;
}
