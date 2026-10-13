// Battle calculations: the adjacency of a unit to the battle, commanders' skills, element strength, odds, ratings,
// range and shots, terrain classes, entry and movement costs, steps on the field, player control and the element at
// a cell.
//
// Table reads go through the data segment's bytes, so an index past a table reads what the original reads.
#include "map/btlcalc.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/btllist.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {

constexpr uint8_t COST_CLASSES{ 8 }; // battle_terrain_costs entries per terrain class
constexpr uint8_t NO_ELEMENT_RANGE{ 20 };
constexpr uint8_t SKILL_CLASS_INFANTRY{ 0 }; // btlcalc_skill_class values
constexpr uint8_t SKILL_CLASS_ARTILLERY{ 1 };
constexpr uint8_t SKILL_CLASS_ARMOR{ 2 };

// The byte at index of a table in the data segment, past its end as well.
template <typename Table>
[[nodiscard]] auto At(DataSegment& data, Table const& table, unsigned index) -> uint8_t {
  return Record<uint8_t>(data, static_cast<uint16_t>(Near(data, table) + index));
}

[[nodiscard]] auto CostOf(DataSegment& data, uint8_t terrain, uint8_t category) -> uint8_t {
  return At(data, data.battle_terrain_costs, terrain * COST_CLASSES + category);
}

// Two's-complement 16-bit wrap of the original's int arithmetic.
[[nodiscard]] auto Wrap16(long value) -> int {
  value &= 0xffffL;
  return static_cast<int>(value >= 0x8000L ? value - 0x10000L : value);
}
} // namespace

void opeu_battle_pause(Game& game, uint8_t ticks) {
  opeu_delay_units(game, ticks);
}

// The grid direction 0..7 whose step from the battle's map cell reaches the unit's cell; BATTLE_NOT_ADJACENT when
// none does.
uint8_t opeu_battle_unit_direction(Game& game, uint16_t unit) {
  auto&       data{ Data(game) };
  auto const& record{ Record<Unit>(data, unit) };
  for (uint8_t direction{ 0 }; direction < 8; ++direction) {
    uint8_t x{ data.battle_x };
    uint8_t y{ data.battle_y };
    if (opeu_grid_step(&x, &y, direction) && record.x == x && record.y == y) return direction;
  }
  return BATTLE_NOT_ADJACENT;
}

// A skill of the commander responsible for an element: the side's commander for the garrison or under a battle
// command, otherwise the division's. The second garrison test is unreachable in the original as well.
uint8_t opeu_battle_skill(Game& game, uint16_t element, uint8_t kind) {
  auto&      data{ Data(game) };
  auto const side{ opeu_battle_element_side(game, element) };
  auto const unit{ opeu_battle_element_unit(game, element) };
  auto const garrison{ Near(data, data.battle_garrison) };
  uint16_t   commander;
  if (data.battle_command[side] || unit == garrison)
    commander = opeu_find_side_record(game, side, 1);
  else if (unit == garrison)
    commander = opeu_find_side_record(game, side, 2);
  else
    commander = opeu_find_group_record(game, side, opeu_unit_group_index(unit));
  auto const& person{ Record<Person>(data, commander) };
  switch (static_cast<BattleSkill>(kind)) {
  case BattleSkill::LEADERSHIP: return person.leadership;
  case BattleSkill::CHARM:      return person.charm;
  case BattleSkill::BRAVERY:    return person.bravery;
  case BattleSkill::HEALTH:     return person.health;
  case BattleSkill::INFANTRY:   return person.infantry;
  case BattleSkill::ARTILLERY:  return person.artillery;
  case BattleSkill::ARMOR:      return person.armor;
  case BattleSkill::EXPERIENCE: return person.experience;
  }
  return 0;
}

uint8_t opeu_battle_element_strength(Game& game, uint16_t element) {
  auto&       data{ Data(game) };
  auto const& type{ ElementTypeOf(data, element) };
  auto const& record{ Record<Element>(data, element) };
  auto        category{ type.category };
  if (category == Bits(Category::ARTILLERY) && type.gun_range <= RANGED_MODE) category = Bits(Category::TANKS);
  auto const skill{ [&](BattleSkill kind) { return opeu_battle_skill(game, element, Bits(kind)); } };
  auto       bonus{ static_cast<uint8_t>(skill(BattleSkill::EXPERIENCE) / 5) };
  switch (At(data, data.btlcalc_skill_class, category)) {
  case SKILL_CLASS_INFANTRY:  opeu_add_byte(skill(BattleSkill::INFANTRY), 99, &bonus); break;
  case SKILL_CLASS_ARTILLERY: opeu_add_byte(skill(BattleSkill::ARTILLERY), 99, &bonus); break;
  case SKILL_CLASS_ARMOR:     opeu_add_byte(skill(BattleSkill::ARMOR), 99, &bonus); break;
  }
  bonus /= 10;
  int strength{ (record.experience / 30 - record.training / 20 + 10) * (At(data, data.battle_type_factors, category) / 10) - bonus };
  if (strength < 5) strength = 5;
  if (strength > 255) strength = 255;
  return static_cast<uint8_t>(strength);
}

uint8_t opeu_battle_odds(Game&, uint8_t rating, uint8_t range) {
  int odds{ rating - range + 30 };
  if (odds <= 0) odds = odds > -15 ? 1 : 0;
  if (odds >= 100) odds = 99;
  return static_cast<uint8_t>(odds);
}

uint8_t opeu_battle_element_rating(Game& game, uint16_t attacker, uint16_t target) {
  auto& data{ Data(game) };
  if (!target || ElementTypeOf(data, target).category != Bits(Category::TANKS)) return ElementTypeOf(data, attacker).anti_personnel;
  return ElementTypeOf(data, attacker).anti_tank;
}

uint8_t opeu_battle_element_range(Game& game, uint16_t element) {
  if (!element) return NO_ELEMENT_RANGE;
  return ElementTypeOf(Data(game), element).defense;
}

uint8_t opeu_battle_element_shots(Game& game, uint16_t attacker, uint16_t target) {
  auto&      data{ Data(game) };
  auto const quantity{ Record<Element>(data, attacker).quantity };
  if (ElementTypeOf(data, attacker).category != Bits(Category::INFANTRY)) return static_cast<uint8_t>(quantity);
  auto const shots{ static_cast<uint8_t>(!target || ElementTypeOf(data, target).category == Bits(Category::INFANTRY) ? quantity / 10 : quantity / 50) };
  return shots ? shots : 1;
}

uint8_t opeu_battle_cell_terrain(Game& game, uint8_t x, uint8_t y) {
  auto&      data{ Data(game) };
  auto const terrain{ static_cast<uint8_t>(BattleTerrainAt(data, x, y) & BATTLE_TERRAIN_MASK) };
  return At(data, data.btlcalc_terrain_class, terrain);
}

uint16_t opeu_battle_can_enter(Game& game, uint8_t x, uint8_t y, uint16_t element) {
  auto&      data{ Data(game) };
  auto const terrain{ opeu_battle_cell_terrain(game, x, y) };
  return CostOf(data, terrain, ElementTypeOf(data, element).category) != IMPASSABLE_COST;
}

// IMPASSABLE_COST for impassable terrain, else a cost clamped to 5..250. The product is the original's 16-bit int
// arithmetic.
uint8_t opeu_battle_move_cost(Game& game, uint8_t x, uint8_t y, uint16_t element) {
  auto&       data{ Data(game) };
  auto const& record{ Record<Element>(data, element) };
  auto const  category{ ElementTypeOf(data, element).category };
  auto const  cost{ CostOf(data, opeu_battle_cell_terrain(game, x, y), category) };
  if (cost == IMPASSABLE_COST) return cost;
  int handling{ opeu_battle_skill(game, element, Bits(BattleSkill::EXPERIENCE)) / 5 };
  handling += opeu_battle_skill(game, element, Bits(BattleSkill::LEADERSHIP));
  handling  = handling / 20 - 2;
  handling += record.training / 20 - 1;
  handling -= record.experience / 20;
  int speed{ ElementTypeOf(data, element).speed };
  if (category == Bits(Category::INFANTRY)) speed <<= 1;
  int result{ (Wrap16(static_cast<long>(Wrap16(static_cast<long>((100 - speed) / 3) * cost)) * 10) / (static_cast<int>(opeu_max16(CostOf(data, 0, category), 1)) * 10) - handling) / 2 };
  if (result < 5) result = 5;
  if (result > 250) result = 250;
  return static_cast<uint8_t>(result);
}

void opeu_battle_step_unchecked(Game& game, uint8_t* x, uint8_t* y, uint8_t direction) {
  auto& data{ Data(game) };
  if (direction < 4) {
    *x += data.btlcalc_step_x[direction] - 1;
    *y += data.btlcalc_step_y[direction] - 1;
  }
}

uint16_t opeu_battle_step(Game& game, uint8_t* x, uint8_t* y, uint8_t direction) {
  uint8_t next_x{ *x };
  uint8_t next_y{ *y };
  opeu_battle_step_unchecked(game, &next_x, &next_y, direction);
  if (next_x < BATTLE_WIDTH && next_y < BATTLE_WIDTH) {
    *x = next_x;
    *y = next_y;
    return 1;
  }
  return 0;
}

uint16_t opeu_battle_unit_controlled(Game& game, uint16_t unit) {
  auto&      data{ Data(game) };
  auto const side{ opeu_battle_unit_side(game, unit) };
  return data.battle_command[side] && data.side_control[side] == SIDE_PLAYED;
}

uint16_t opeu_battle_element_at(Game& game, uint8_t x, uint8_t y) {
  auto& data{ Data(game) };
  for (uint8_t side{ 0 }; side < 2; ++side)
    for (uint8_t i{ 0 }; i < BATTLE_LIST_LENGTH; ++i)
      if (auto const unit{ BattleList(data, side, i) })
        for (auto& element : Elements(data, Record<Unit>(data, unit))) {
          auto const& position{ PositionOf(data, Near(data, element)) };
          if (position.x == x && position.y == y) return Near(data, element);
        }
  return 0;
}
