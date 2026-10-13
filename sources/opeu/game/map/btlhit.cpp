// Battle attack damage and its application: the losses of the target, the attacker's gains and the damage that
// grows with the losses; a target hit without an attacker takes mine damage.
//
// Every product and quotient is truncated to 16 bits as the original's MUL/DIV pairs do.
#include "map/btlhit.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t TRAINING_GAIN{ 1 };
constexpr uint8_t MORALE_GAIN{ 3 };

[[nodiscard]] auto U16(unsigned v) -> uint16_t {
  return static_cast<uint16_t>(v);
}

void AddByte(uint8_t& at, uint16_t amount, uint16_t limit) {
  at = static_cast<uint8_t>(opeu_min16(U16(amount + at), limit));
}
void AddWord(uint16_t& at, uint16_t amount, uint16_t limit) {
  auto const sum{ static_cast<uint32_t>(amount) + at };
  at = U16(sum > limit ? limit : sum);
}
void SubWord(uint16_t& at, uint16_t amount) {
  at = U16(at - opeu_min16(at, amount));
}

// Types whose fire is indirect: categories 6 and 7, and heavy artillery.
[[nodiscard]] auto Indirect(ElementType const& type) -> bool {
  return type.category == 7 || type.category == 6 || (type.category == std::to_underlying(Category::ARTILLERY) && type.flags & Bits(TypeFlag::HEAVY));
}

// After a loss from `old`, the element spends its supplies.
void SpendSupplies(uint8_t* ds, BattleEvent event, uint16_t old, uint16_t element) {
  Call(ds, event, BattleRoutine::BATTLE_SPEND_FUEL, old, element);
  Call(ds, event, BattleRoutine::BATTLE_SPEND_AMMO, old, element);
  Call(ds, event, BattleRoutine::BATTLE_SPEND_STORES, old, element);
}
} // namespace

uint16_t opeu_battle_attack_damage(uint8_t* ds, BattleEvent event, uint16_t attacker, uint16_t target) {
  auto&      data{ Data(ds) };
  auto const company{ Call(ds, event, BattleRoutine::BATTLE_ELEMENT_UNIT, attacker) };
  auto const shots{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_SHOTS, target, attacker)) };
  SubWord(Record<Unit>(data, company).resources[0], U16(shots / 3));
  auto const* model{ &ElementTypeOf(data, attacker) };
  uint16_t    headcount{ Record<Element>(data, attacker).quantity };
  if (IsOfCategory(*model, Category::INFANTRY)) headcount /= 10;
  if (target == 0) return U16(U16(Call(ds, event, BattleRoutine::ISQRT, model->anti_tank) * static_cast<uint32_t>(headcount)) / 100);
  if (Call(ds, event, BattleRoutine::BATTLE_FIRE_HITS, target, attacker) == 0) return 0;
  auto const strength{ Call(ds, event, BattleRoutine::BATTLE_FIRE_POWER, target, attacker) };
  auto const defense{ Call(ds, event, BattleRoutine::BATTLE_FIRE_DEFENSE, target) };
  auto const odds{ U16(U16(strength * 10u) / opeu_max16(U16(Call(ds, event, BattleRoutine::RAND_BOUNDED, 2) + defense), 1)) };
  headcount = U16(headcount / 10 + 1);
  model     = &ElementTypeOf(data, target);
  auto const quantity{ Record<Element>(data, target).quantity };
  auto const size{ U16(IsOfCategory(*model, Category::INFANTRY) ? quantity / 100 + 1 : quantity / 10 + 1) };
  auto const vision{ opeu_max16(U16(data.weather_obscurity[data.weather] / 3 + data.battle_visibility / 5 + 2), 1) };
  if (Indirect(*model)) {
    uint16_t const night{ U16(opeu_hour_is_night(data.hour) != 0 ? 2 : 1) };
    auto const     loss{ U16(U16(U16(odds / 10 * headcount) / 7) / night / vision) };
    auto const     bonus{ Call(ds, event, BattleRoutine::RAND_BOUNDED, 2) };
    uint16_t const twice{ U16(Call(ds, event, BattleRoutine::RAND_BOUNDED, 6) == 0 ? 2 : 1) };
    return U16(bonus + twice * loss);
  }
  uint16_t const night{ U16(opeu_hour_is_night(data.hour) != 0 ? 2 : 1) };
  auto const     loss{ U16(U16(odds * static_cast<uint32_t>(headcount)) / night / vision * size) };
  auto const     bonus{ Call(ds, event, BattleRoutine::RAND_BOUNDED, 2) };
  uint16_t const twice{ U16(Call(ds, event, BattleRoutine::RAND_BOUNDED, 6) == 0 ? 2 : 1) };
  return U16(bonus + U16(loss / (IsOfCategory(*model, Category::INFANTRY) ? 7 : 70) * twice));
}

void opeu_battle_apply_damage(uint8_t* ds, BattleEvent event, uint16_t attacker, uint16_t target) {
  auto&      data{ Data(ds) };
  auto const damage{ opeu_battle_attack_damage(ds, event, attacker, target) };
  data.battle_element_destroyed = 0;
  if (damage != 0 && target != 0) {
    auto&      hit{ Record<Element>(data, target) };
    auto const old{ hit.quantity };
    SubWord(hit.quantity, damage);
    SpendSupplies(ds, event, old, target);
    if (hit.quantity == 0) {
      data.battle_element_destroyed = 1;
      auto& winner{ Record<Element>(data, attacker) };
      AddByte(winner.training, TRAINING_GAIN, RATING_LIMIT);
      AddByte(winner.morale, MORALE_GAIN, RATING_LIMIT);
    } else if (auto const category{ data.element_types[hit.type].category }; category != 0) {
      auto const roll{ Call(ds, event, BattleRoutine::RAND_BOUNDED, 300) };
      auto const rate{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_TYPE_SKILL, target)) };
      AddWord(hit.damage, U16(U16(rate * damage) / U16(700 - roll)), data.request_capacities[category]);
    }
  }
  Call(ds, event, BattleRoutine::BATTLE_SET_ORDER, 0, attacker);
}

void opeu_battle_apply_bombard(uint8_t* ds, BattleEvent event, uint16_t target) {
  auto&      data{ Data(ds) };
  auto const damage{ Call(ds, event, BattleRoutine::BATTLE_MINE_DAMAGE, target) };
  data.battle_element_destroyed = 0;
  if (damage != 0) {
    auto&      hit{ Record<Element>(data, target) };
    auto const old{ hit.quantity };
    SubWord(hit.quantity, damage);
    SpendSupplies(ds, event, old, target);
    if (hit.quantity == 0) {
      hit.damage                    = 0;
      data.battle_element_destroyed = 1;
    } else if (auto const category{ data.element_types[hit.type].category }; category != 0) {
      auto const amount{ Call(ds, event, BattleRoutine::RAND_BOUNDED, damage >> 1) };
      AddWord(hit.damage, amount, data.request_capacities[category]);
    }
  }
  Call(ds, event, BattleRoutine::BATTLE_SET_ORDER, 0, target);
}

void opeu_battle_resolve_attack(uint8_t* ds, BattleEvent event, uint16_t attacker, uint16_t target) {
  if (attacker != 0)
    opeu_battle_apply_damage(ds, event, attacker, target);
  else
    opeu_battle_apply_bombard(ds, event, target);
  if (Data(ds).battle_element_destroyed != 0) Call(ds, event, BattleRoutine::BATTLE_ELEMENT_LOST, target);
}

uint16_t svc_btlhit(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_resolve_attack(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_battle_resolve_attack(ds, svc_btlhit, Arg(args, count, 1), Arg(args, count, 0));
  return 0;
}
