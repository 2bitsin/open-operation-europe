// Battle retreats: the cells a retreating unit may enter, the retreat itself and the per-round morale pass.
//
// A retreat cell holds no unit, an objective only when held by the side of the second list's lead unit, terrain
// every element can cross, and no mark of the unit's side. Retreating into a cell the enemy marked costs a random
// element the bombard path of the attack resolution. A garrison that breaks damages its objective instead.
#include "map/btlretr.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/numeric.hpp"
#include "core/random.hpp"
#include "map/btllist.hpp"
#include "map/btlmove.hpp"
#include "map/cellflag.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// BATTLE_UNIT_DIRECTION of a unit standing on the battle cell; the battle direction before any retreat.
constexpr uint8_t AT_BATTLE{ 9 };
constexpr uint8_t NO_DIRECTION{ 0xff };
constexpr uint8_t DIRECTIONS{ 8 };
constexpr uint8_t IMPASSABLE{ 0xff };
constexpr uint8_t MOVE_CLASSES{ 5 }; // move_costs columns

// The unit's slot in its division: its index in units (signed 16-bit division), low three bits.
[[nodiscard]] auto UnitSlot(uint16_t unit) -> uint8_t {
  return static_cast<uint8_t>(static_cast<int16_t>(static_cast<uint16_t>(unit - UNITS_AT)) / static_cast<int16_t>(sizeof(Unit)) & 7);
}

[[nodiscard]] auto ComputerTurn(uint8_t* ds, BattleEvent event) -> uint8_t {
  return static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_COMPUTER_TURN));
}

[[nodiscard]] auto Direction(uint8_t* ds, BattleEvent event, uint16_t unit) -> uint8_t {
  return static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_DIRECTION, unit));
}

// Takes `percent` of a stock away (the stock field is packed, so it is worked on as a copy).
void Reduce(uint16_t& stock, uint16_t percent) {
  uint16_t value{ stock };
  opeu_sub_word(opeu_mul_div(value, percent, 100), &value);
  stock = value;
}
} // namespace

uint16_t opeu_battle_retreat_cell_ok(uint8_t* ds, BattleEvent event, uint8_t x, uint8_t y, uint16_t unit) {
  auto& data{ Data(ds) };
  if (opeu_find_unit_at(ds, x, y)) return 0;
  if (auto const objective{ opeu_find_objective_at(ds, x, y) }) {
    auto const side{ opeu_battle_unit_side(ds, event, BattleList(data, 1, 0)) };
    if (side != ((Record<Objective>(data, objective).flags & Bits(ObjectiveFlag::SIDE_1)) != 0)) return 0;
  }
  auto const terrain{ MapCellAt(data, x, y) & Bits(MapCell::TERRAIN_MASK) };
  for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
    auto const move_class{ data.battle_retreat_class[data.element_types[element.type].category] };
    if (data.move_costs[terrain * MOVE_CLASSES + move_class] == IMPASSABLE) return 0;
  }
  if (opeu_cell_marked(ds, x, y, opeu_battle_unit_side(ds, event, unit))) return 0;
  return 1;
}

// A random element of the unit takes the bombard path of the attack resolution, and the unit's side marks the cell.
void opeu_battle_retreat_to(uint8_t* ds, BattleEvent event, uint16_t unit, uint8_t x, uint8_t y) {
  auto&    data{ Data(ds) };
  auto&    record{ Record<Unit>(data, unit) };
  uint16_t count{ 0 };
  for (auto it{ Elements(data, record).begin() }; it != std::default_sentinel; ++it) ++count;
  auto const chosen{ opeu_rand_bounded(count) };
  auto       it{ Elements(data, record).begin() };
  for (uint16_t i{ 0 }; chosen > i; ++i) ++it;
  Call(ds, event, BattleRoutine::BATTLE_RESOLVE_ATTACK, it.Near(), 0);
  MapCellAt(data, x, y) |= opeu_battle_unit_side(ds, event, unit) ? Bits(MapCell::MARK_1) : Bits(MapCell::MARK_0);
}

// side: the unit's battle list (unused).
void opeu_battle_unit_retreat(uint8_t* ds, BattleEvent event, uint16_t unit, uint8_t) {
  auto& data{ Data(ds) };
  if (Direction(ds, event, unit) == AT_BATTLE && unit != GARRISON_AT) {
    auto const side{ opeu_battle_unit_side(ds, event, unit) };
    for (uint8_t direction{ 0 }; direction < DIRECTIONS; ++direction) {
      uint8_t x{ data.battle_x };
      uint8_t y{ data.battle_y };
      if (!opeu_grid_step(&x, &y, direction)) continue;
      if (!opeu_battle_retreat_cell_ok(ds, event, x, y, unit)) continue;
      uint8_t to_x{ data.battle_x };
      uint8_t to_y{ data.battle_y };
      opeu_grid_step(&to_x, &to_y, direction);
      if (opeu_cell_marked(ds, to_x, to_y, 1)) {
        if (opeu_cell_marked(ds, to_x, to_y, side)) continue;
        opeu_battle_retreat_to(ds, event, unit, to_x, to_y);
        return;
      }
      data.battle_direction = direction;
    }
    if (data.battle_direction != NO_DIRECTION) opeu_battle_remove_list(ds, event, unit);
  }
  if (Direction(ds, event, unit) != AT_BATTLE) opeu_battle_remove_list(ds, event, unit);
  if (unit == GARRISON_AT) {
    auto&      objective{ Record<Objective>(data, opeu_find_objective_at(ds, data.battle_x, data.battle_y)) };
    auto const elements{ opeu_battle_element_count(ds, event, unit) };
    auto const percent{ opeu_rand_bounded(static_cast<uint16_t>(elements * 10u + 0x29)) };
    Reduce(objective.resources[1], percent);
    Reduce(objective.resources[2], percent);
    Reduce(objective.resources[0], percent);
    for (auto& item : objective.items)
      if (item.quantity) item.quantity = 0;
    opeu_battle_remove_list(ds, event, unit);
  }
}

// Units out of ammunition may retreat; a side outnumbered more than five to one may break; supply and repair units
// retreat more readily the more retreat cells they have.
void opeu_battle_morale_check(uint8_t* ds, BattleEvent event) {
  auto&   data{ Data(ds) };
  uint8_t strength[SIDE_COUNT];
  for (uint8_t list{ 0 }; list < SIDE_COUNT; ++list) {
    strength[list] = 0;
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
      if (auto const unit{ BattleList(data, list, slot) }) {
        if (!Record<Unit>(data, unit).resources[0] && opeu_rand_bounded(100) < 30) opeu_battle_unit_retreat(ds, event, unit, list);
        strength[list] = static_cast<uint8_t>(strength[list] + opeu_battle_element_count(ds, event, unit));
      }
    if (!strength[list]) return;
  }
  uint8_t odds;
  uint8_t weaker;
  if (strength[1] < strength[0]) {
    odds   = static_cast<uint8_t>(strength[0] / strength[1]);
    weaker = 1;
  } else {
    odds   = static_cast<uint8_t>(strength[1] / strength[0]);
    weaker = 0;
  }
  auto const support{ [](uint16_t unit) {
    auto const slot{ UnitSlot(unit) };
    return opeu_category_6(slot) || opeu_category_7(slot);
  } };
  if (odds > 5)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
      if (auto const unit{ BattleList(data, weaker, slot) }; unit && unit != GARRISON_AT && !support(unit) && opeu_rand_bounded(100) < 20) {
        opeu_battle_unit_retreat(ds, event, unit, weaker);
        if (ComputerTurn(ds, event)) return;
      }
  for (uint8_t list{ 0 }; list < SIDE_COUNT; ++list)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot) {
      auto const unit{ BattleList(data, list, slot) };
      if (!unit || unit == GARRISON_AT || !support(unit)) continue;
      uint8_t chance{ 20 };
      for (uint8_t direction{ 0 }; direction < DIRECTIONS; ++direction) {
        uint8_t x{ data.battle_x };
        uint8_t y{ data.battle_y };
        if (opeu_grid_step(&x, &y, direction) && opeu_battle_retreat_cell_ok(ds, event, x, y, unit)) chance = static_cast<uint8_t>(chance + 10);
      }
      if (opeu_rand_bounded(100) < chance) {
        opeu_battle_unit_retreat(ds, event, unit, list);
        if (ComputerTurn(ds, event)) return;
      }
    }
}

uint16_t svc_btlretr(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_morale_check(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_battle_morale_check(ds, svc_btlretr);
  return 0;
}
