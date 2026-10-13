// Battle fire: the hit roll, the attacker's fire power and the target's defense.
//
// The comparisons keep the original's operand order and every product wraps at 16 bits as MUL does.
#include "map/btlfire.hpp"
#include "common/battle-calls.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/people.hpp"
#include "map/btllist.hpp"
#include "map/btlrcalc.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The kinds of battle skill opeu_battle_skill rates.
constexpr uint8_t  SKILL_0{ 0 };
constexpr uint8_t  SKILL_2{ 2 };
constexpr uint8_t  SKILL_7{ 7 };
constexpr uint16_t CONDITION_BASE{ 0x68 };
constexpr uint8_t  SIDE_RECORD_COMMANDER{ 1 };

[[nodiscard]] auto Mul16(unsigned x, unsigned y) -> uint16_t {
  return static_cast<uint16_t>(static_cast<uint16_t>(x) * static_cast<uint32_t>(static_cast<uint16_t>(y)));
}

[[nodiscard]] auto Quality(Game& game, uint16_t element) -> uint8_t {
  return static_cast<uint8_t>(opeu_battle_type_skill(game, element));
}
[[nodiscard]] auto SideOf(Game& game, uint16_t element) -> uint8_t {
  return static_cast<uint8_t>(opeu_battle_element_side(game, element));
}

// Whether the unit's commander rides with the element (its selected element).
[[nodiscard]] auto Led(DataSegment& data, uint16_t element) -> uint16_t {
  return (Record<Element>(data, element).flags & Bits(ElementFlag::SELECTED)) != 0;
}

// 1 + the type's mode / 2: a factor of the type's fire.
[[nodiscard]] auto ModeFactor(ElementType const& type) -> unsigned {
  return (type.gun_range >> 1) + 1u;
}
} // namespace

// 1 when a 0..99 draw falls under the hit chance.
uint16_t opeu_battle_fire_hits(Game& game, uint16_t attacker, uint16_t target) {
  auto&       data{ Data(game) };
  auto const& a{ Record<Element>(data, attacker) };
  auto const& t{ Record<Element>(data, target) };
  uint16_t    chance;
  uint8_t     x;
  uint8_t     y;
  x      = ElementSkill(game, attacker, SKILL_0);
  y      = ElementSkill(game, target, SKILL_0);
  chance = x < y ? 5 : 10;
  x      = ElementSkill(game, attacker, SKILL_2);
  y      = ElementSkill(game, target, SKILL_2);
  chance = static_cast<uint16_t>(chance + (x < y ? 5 : 10));
  x      = ElementSkill(game, attacker, SKILL_7);
  y      = ElementSkill(game, target, SKILL_7);
  chance = static_cast<uint16_t>(chance + (x < y ? 5 : 10));
  x      = Quality(game, attacker);
  y      = Quality(game, target);
  chance = static_cast<uint16_t>(chance + (x < y ? 10 : 20));
  chance = static_cast<uint16_t>(chance + (a.morale < t.morale ? 5 : 10));
  chance = static_cast<uint16_t>(chance + (a.training < t.training ? 5 : 10));
  chance = static_cast<uint16_t>(chance + (a.experience > t.experience ? 5 : 10));
  chance = static_cast<uint16_t>(chance + ModeFactor(ElementTypeOf(data, attacker)) * 5);
  chance = static_cast<uint16_t>(chance - Mul16(data.weather_obscurity[data.weather] + data.battle_visibility, opeu_hour_is_night(data.hour) ? 25 : 15) / 10);
  return game.State().RandomBounded(100) < chance ? 1 : 0;
}

// The attacker's fire power against the target: the soft rating against infantry, the hard rating otherwise.
uint16_t opeu_battle_fire_power(Game& game, uint16_t attacker, uint16_t target) {
  auto&          data{ Data(game) };
  auto const&    type{ ElementTypeOf(data, attacker) };
  auto const&    a{ Record<Element>(data, attacker) };
  uint16_t const rating{ ElementTypeOf(data, target).category == std::to_underlying(Category::INFANTRY) ? type.anti_personnel : type.anti_tank };
  uint16_t const skill_2{ ElementSkill(game, attacker, SKILL_2) };
  uint16_t const grade{ Quality(game, attacker) };
  uint16_t const skill_7{ ElementSkill(game, attacker, SKILL_7) };
  auto const     record{ opeu_find_side_record(game, SideOf(game, attacker), SIDE_RECORD_COMMANDER) };
  auto const     lead{ Led(data, attacker) };
  auto           value{ static_cast<uint16_t>(a.training - a.experience + skill_7 + a.morale + grade + skill_2 + CONDITION_BASE) };
  value = static_cast<uint16_t>(Mul16(value / 5, rating) / 100);
  value = Mul16(value, Record<Person>(data, record).leadership / 85 * lead + 1);
  return Mul16(value, ModeFactor(type));
}

// The element's defense against fire (the type's range byte serves as its armour), with a random bonus that grows
// with the delivery level while the computer commands the element's side.
uint16_t opeu_battle_fire_defense(Game& game, uint16_t element) {
  auto&          data{ Data(game) };
  auto const&    e{ Record<Element>(data, element) };
  uint16_t const armour{ ElementTypeOf(data, element).defense };
  uint16_t const skill_2{ ElementSkill(game, element, SKILL_2) };
  uint16_t const grade{ Quality(game, element) };
  uint16_t const skill_7{ ElementSkill(game, element, SKILL_7) };
  auto const     record{ opeu_find_side_record(game, SideOf(game, element), SIDE_RECORD_COMMANDER) };
  auto const     lead{ Led(data, element) };
  auto           value{ static_cast<uint16_t>(e.training - e.experience + e.morale + skill_7 + grade + skill_2 + CONDITION_BASE) };
  value = static_cast<uint16_t>(Mul16(value / 5, armour) / 100);
  auto const side{ SideOf(game, element) };
  auto const bonus{ game.State().RandomBounded(data.side_control[side] == SIDE_COMPUTER ? data.delivery_level + 1 : 1) };
  return static_cast<uint16_t>(Mul16(Record<Person>(data, record).leadership / 70 * lead + 1, value) + bonus + 1);
}
