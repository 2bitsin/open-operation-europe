// Ranged (artillery) fire ratings: the element's skill for its type, the hit roll, the ranged power and defence,
// and the casualties of one shot (a target in a bunker takes a fixed barrage).
#include "map/btlrcalc.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/people.hpp"
#include "core/random.hpp"
#include "map/btldir.hpp"
#include "map/btllist.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The skill kinds BATTLE_SKILL rates (person and type skills).
constexpr uint8_t SKILL_COMMAND{ 0 };
constexpr uint8_t SKILL_TACTICS{ 2 };
constexpr uint8_t SKILL_FIRE{ 7 };
constexpr uint8_t TYPE_SKILLS[]{ 4, 5, 6 }; // by battle_skill_kinds
// The bonus base of every rating, and the leader's scale for power and defence.
constexpr uint16_t RATING_BASE{ 0x68 };
constexpr uint8_t  POWER_LEADERSHIP{ 0x55 };
constexpr uint8_t  DEFENCE_LEADERSHIP{ 0x46 };

// 16-bit multiplication, as MUL keeps the low word.
[[nodiscard]] constexpr auto Mul16(uint16_t a, uint16_t b) -> uint16_t {
  return static_cast<uint16_t>(static_cast<uint32_t>(a) * b);
}

[[nodiscard]] auto Skill(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t kind) -> uint8_t {
  return static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_SKILL, kind, element));
}

[[nodiscard]] auto TypeOf(DataSegment const& data, uint16_t element) -> ElementType const& {
  return data.element_types[Record<Element>(data, element).type];
}

[[nodiscard]] auto Is(ElementType const& type, Category category) -> bool {
  return type.category == Bits(category);
}

// The cover of the target's cell; weather receives the weather's obscurity. The cover table runs on from
// battle_terrain_cover, one byte per battle terrain.
[[nodiscard]] auto Cover(uint8_t* ds, BattleEvent event, uint16_t target, uint16_t& weather) -> uint16_t {
  auto&       data{ Data(ds) };
  auto const& position{ PositionOf(data, target) };
  auto const  terrain{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_CELL_TERRAIN, position.x, position.y)) };
  weather = data.weather_obscurity[data.weather];
  return Record<uint8_t>(data, static_cast<uint16_t>(Near(data, data.battle_terrain_cover) + terrain));
}

[[nodiscard]] auto Night(DataSegment const& data) -> bool {
  return opeu_hour_is_night(data.hour);
}

// The side's commander's leadership, counting only for the unit's lead element.
[[nodiscard]] auto Leadership(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t scale) -> uint16_t {
  auto&          data{ Data(ds) };
  auto const     commander{ opeu_find_side_record(ds, opeu_battle_element_side(ds, event, element), 1) };
  uint16_t const lead{ (Record<Element>(data, element).flags & Bits(ElementFlag::SELECTED)) != 0 };
  return static_cast<uint16_t>(Record<Person>(data, commander).leadership / scale * lead + 1);
}
} // namespace

// The element's skill for its type's skill kind (0 when none).
uint8_t opeu_battle_type_skill(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto const& data{ Data(ds) };
  auto const  kind{ data.battle_skill_kinds[TypeOf(data, element).category] };
  if (kind < std::size(TYPE_SKILLS)) return Skill(ds, event, element, TYPE_SKILLS[kind]);
  return 0;
}

// 1 when a 0..99 draw falls under the ranged hit chance.
uint16_t opeu_battle_ranged_hits(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target) {
  auto&      data{ Data(ds) };
  uint16_t   chance{ 0 };
  auto const better{ [&](uint8_t kind, uint16_t yes, uint16_t no) {
    auto const own{ Skill(ds, event, shooter, kind) };
    auto const other{ Skill(ds, event, target, kind) };
    chance = static_cast<uint16_t>(chance + (own >= other ? yes : no));
  } };
  better(SKILL_COMMAND, 10, 5);
  better(SKILL_TACTICS, 10, 5);
  better(SKILL_FIRE, 10, 5);
  auto const own_type{ opeu_battle_type_skill(ds, event, shooter) };
  auto const other_type{ opeu_battle_type_skill(ds, event, target) };
  chance = static_cast<uint16_t>(chance + (own_type >= other_type ? 20 : 10));
  auto const& own{ Record<Element>(data, shooter) };
  auto const& other{ Record<Element>(data, target) };
  chance = static_cast<uint16_t>(chance + (own.morale >= other.morale ? 10 : 5));
  chance = static_cast<uint16_t>(chance + (own.training >= other.training ? 10 : 5));
  chance = static_cast<uint16_t>(chance + (own.experience > other.experience ? 5 : 10));
  auto const mode{ opeu_battle_terrain_mode(ds, event, shooter, target) };
  // Signed halving: the range can fall short of the distance.
  chance = static_cast<uint16_t>(chance + static_cast<uint16_t>((static_cast<int>(TypeOf(data, shooter).gun_range) - mode + 1) / 2 + 1) * 5u);
  uint16_t   weather;
  auto const cover{ Cover(ds, event, target, weather) };
  chance = static_cast<uint16_t>(chance - Mul16(static_cast<uint16_t>(cover + weather), Night(data) ? 25 : 15) / 10);
  return opeu_rand_bounded(100) < chance ? 1 : 0;
}

// The shooter's ranged power against the target.
uint16_t opeu_battle_ranged_power(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target) {
  auto&          data{ Data(ds) };
  auto const&    type{ TypeOf(data, shooter) };
  auto const&    own{ Record<Element>(data, shooter) };
  uint16_t const rating{ Is(TypeOf(data, target), Category::INFANTRY) ? type.anti_personnel : type.anti_tank };
  uint16_t const tactics{ Skill(ds, event, shooter, SKILL_TACTICS) };
  uint16_t const grade{ opeu_battle_type_skill(ds, event, shooter) };
  uint16_t const fire{ Skill(ds, event, shooter, SKILL_FIRE) };
  auto const     leadership{ Leadership(ds, event, shooter, POWER_LEADERSHIP) };
  auto           value{ static_cast<uint16_t>(grade - own.experience + tactics + own.training + own.morale + fire + RATING_BASE) };
  value = static_cast<uint16_t>(Mul16(value / 5, rating) / 100);
  auto const reach{ static_cast<uint16_t>(type.gun_range - opeu_battle_terrain_mode(ds, event, shooter, target) + 1) };
  value = Mul16(value, static_cast<uint16_t>((reach >> 1) + 1));
  return Mul16(value, leadership);
}

// The element's defence against ranged fire, with a random bonus that grows with the delivery level on a side the
// computer commands.
uint16_t opeu_battle_ranged_defense(uint8_t* ds, BattleEvent event, uint16_t element) {
  auto&          data{ Data(ds) };
  auto const&    own{ Record<Element>(data, element) };
  uint16_t const armour{ TypeOf(data, element).defense };
  uint16_t const tactics{ Skill(ds, event, element, SKILL_TACTICS) };
  uint16_t const grade{ opeu_battle_type_skill(ds, event, element) };
  uint16_t const fire{ Skill(ds, event, element, SKILL_FIRE) };
  auto const     leadership{ Leadership(ds, event, element, DEFENCE_LEADERSHIP) };
  auto           value{ static_cast<uint16_t>(own.training - own.experience + own.morale + fire + tactics + grade + RATING_BASE) };
  value = static_cast<uint16_t>(Mul16(value / 5, armour) / 100);
  auto const side{ opeu_battle_element_side(ds, event, element) };
  auto const bonus{ opeu_rand_bounded(static_cast<uint16_t>(data.side_control[side] == 0 ? data.delivery_level + 1 : 1)) };
  return static_cast<uint16_t>(Mul16(leadership, value) + bonus + 1);
}

// The casualties of one ranged shot. bunker: the DS offset of a word set when there is no target or it stands in a
// bunker (a garrisoned city cell); such a shot is a fixed barrage. Otherwise a hit scales by power, defence, cover,
// weather and night. The shooter's unit spends a third of its shots in ammunition.
uint16_t opeu_battle_ranged_damage(uint8_t* ds, BattleEvent event, uint16_t shooter, uint16_t target, uint16_t bunker) {
  auto& data{ Data(ds) };
  if (!target) {
    SetWord(ds, bunker, 1);
  } else {
    SetWord(ds, bunker, 0);
    auto const& position{ PositionOf(data, target) };
    for (unsigned i{ 0 }; i < BATTLE_EXTRA_COUNT; ++i) {
      auto const& extra{ BattleExtraAt(data, i) };
      if (extra.x == BATTLE_UNPLACED) break;
      if (position.x == extra.x && position.y == extra.y && extra.count) {
        SetWord(ds, bunker, 1);
        break;
      }
    }
  }
  auto&      unit{ Record<Unit>(data, opeu_battle_element_unit(ds, event, shooter)) };
  auto const shots{ static_cast<uint16_t>(static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_SHOTS, target, shooter)) / 3u) };
  auto const ammunition{ unit.resources[0] };
  unit.resources[0] = static_cast<uint16_t>(ammunition - (ammunition < shots ? ammunition : shots));
  auto const& type{ TypeOf(data, shooter) };
  uint16_t    count{ Record<Element>(data, shooter).quantity };
  if (Is(type, Category::INFANTRY)) count /= 10;
  if (Word(ds, bunker)) return static_cast<uint16_t>(Mul16(Call(ds, event, BattleRoutine::ISQRT, type.anti_tank), count) / 100);
  if (!opeu_battle_ranged_hits(ds, event, shooter, target)) return 0;
  auto const power{ opeu_battle_ranged_power(ds, event, shooter, target) };
  auto const defence{ opeu_battle_ranged_defense(ds, event, target) };
  auto       grade{ opeu_rand_bounded(2) };
  grade = static_cast<uint16_t>(grade + Mul16(power, 10) / opeu_max16(defence, 1));
  count = static_cast<uint16_t>(count / 10 + 1);
  auto const& target_type{ TypeOf(data, target) };
  uint16_t    size{ Record<Element>(data, target).quantity };
  size = static_cast<uint16_t>((Is(target_type, Category::INFANTRY) ? size / 100 : size / 10) + 1);
  uint16_t   weather;
  auto const cover{ Cover(ds, event, target, weather) };
  auto const spread{ opeu_max16(static_cast<uint16_t>(cover / 5 + weather / 3 + 2), 1) };
  auto const night{ Night(data) ? 2 : 1 };
  if (Is(target_type, Category::SUPPLY) || Is(target_type, Category::REPAIR) || (Is(target_type, Category::ARTILLERY) && (target_type.flags & Bits(TypeFlag::HEAVY)))) {
    auto const base{ static_cast<uint16_t>(Mul16(grade / 10, count) / 7 / night / spread) };
    auto const roll{ opeu_rand_bounded(2) };
    return static_cast<uint16_t>(roll + Mul16(opeu_rand_bounded(6) == 0 ? 2 : 1, base));
  }
  auto const base{ Mul16(static_cast<uint16_t>(Mul16(grade, count) / night / spread), size) };
  auto const roll{ opeu_rand_bounded(2) };
  return static_cast<uint16_t>(roll + Mul16(static_cast<uint16_t>(base / (Is(target_type, Category::INFANTRY) ? 7 : 70)), opeu_rand_bounded(6) == 0 ? 2 : 1));
}

uint16_t svc_btlrcalc(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_type_skill(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_battle_type_skill(ds, svc_btlrcalc, Arg(args, count, 0));
}
