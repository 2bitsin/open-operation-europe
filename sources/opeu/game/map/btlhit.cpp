// Battle attack damage and its application: the losses of the target, the attacker's gains and the damage that
// grows with the losses; a target hit without an attacker takes mine damage.
//
// Every product and quotient is truncated to 16 bits as the original's MUL/DIV pairs do.
#include "map/btlhit.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "map/btlcalc.hpp"
#include "map/btldepty.hpp"
#include "map/btlfire.hpp"
#include "map/btllist.hpp"
#include "map/btlmines.hpp"
#include "map/btlrcalc.hpp"
#include "map/btlspend.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
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
void SpendSupplies(Game& game, uint16_t old, uint16_t element) {
  opeu_battle_spend_fuel(game, element, old);
  opeu_battle_spend_ammo(game, element, old);
  opeu_battle_spend_stores(game, element, old);
}
} // namespace

uint16_t opeu_battle_attack_damage(Game& game, uint16_t attacker, uint16_t target) {
  auto&      data{ Data(game) };
  auto const company{ opeu_battle_element_unit(game, attacker) };
  auto const shots{ static_cast<uint8_t>(opeu_battle_element_shots(game, svc_btlcalc, attacker, target)) };
  SubWord(Record<Unit>(data, company).resources[0], U16(shots / 3));
  auto const* model{ &ElementTypeOf(data, attacker) };
  uint16_t    headcount{ Record<Element>(data, attacker).quantity };
  if (IsOfCategory(*model, Category::INFANTRY)) headcount /= 10;
  if (target == 0) return U16(U16(opeu_isqrt(static_cast<std::uint16_t>(model->anti_tank)) * static_cast<uint32_t>(headcount)) / 100);
  if (opeu_battle_fire_hits(game, attacker, target) == 0) return 0;
  auto const strength{ opeu_battle_fire_power(game, attacker, target) };
  auto const defense{ opeu_battle_fire_defense(game, target) };
  auto const odds{ U16(U16(strength * 10u) / opeu_max16(U16(port_rand_bounded(game, 2) + defense), 1)) };
  headcount = U16(headcount / 10 + 1);
  model     = &ElementTypeOf(data, target);
  auto const quantity{ Record<Element>(data, target).quantity };
  auto const size{ U16(IsOfCategory(*model, Category::INFANTRY) ? quantity / 100 + 1 : quantity / 10 + 1) };
  auto const vision{ opeu_max16(U16(data.weather_obscurity[data.weather] / 3 + data.battle_visibility / 5 + 2), 1) };
  if (Indirect(*model)) {
    uint16_t const night{ U16(opeu_hour_is_night(data.hour) != 0 ? 2 : 1) };
    auto const     loss{ U16(U16(U16(odds / 10 * headcount) / 7) / night / vision) };
    auto const     bonus{ port_rand_bounded(game, 2) };
    uint16_t const twice{ U16(port_rand_bounded(game, 6) == 0 ? 2 : 1) };
    return U16(bonus + twice * loss);
  }
  uint16_t const night{ U16(opeu_hour_is_night(data.hour) != 0 ? 2 : 1) };
  auto const     loss{ U16(U16(odds * static_cast<uint32_t>(headcount)) / night / vision * size) };
  auto const     bonus{ port_rand_bounded(game, 2) };
  uint16_t const twice{ U16(port_rand_bounded(game, 6) == 0 ? 2 : 1) };
  return U16(bonus + U16(loss / (IsOfCategory(*model, Category::INFANTRY) ? 7 : 70) * twice));
}

void opeu_battle_apply_damage(Game& game, uint16_t attacker, uint16_t target) {
  auto&      data{ Data(game) };
  auto const damage{ opeu_battle_attack_damage(game, attacker, target) };
  data.battle_element_destroyed = 0;
  if (damage != 0 && target != 0) {
    auto&      hit{ Record<Element>(data, target) };
    auto const old{ hit.quantity };
    SubWord(hit.quantity, damage);
    SpendSupplies(game, old, target);
    if (hit.quantity == 0) {
      data.battle_element_destroyed = 1;
      auto& winner{ Record<Element>(data, attacker) };
      AddByte(winner.training, TRAINING_GAIN, RATING_LIMIT);
      AddByte(winner.morale, MORALE_GAIN, RATING_LIMIT);
    } else if (auto const category{ data.element_types[hit.type].category }; category != 0) {
      auto const roll{ port_rand_bounded(game, 300) };
      auto const rate{ static_cast<uint8_t>(opeu_battle_type_skill(game, target)) };
      AddWord(hit.damage, U16(U16(rate * damage) / U16(700 - roll)), data.request_capacities[category]);
    }
  }
  opeu_battle_set_order(game, attacker, static_cast<uint8_t>(0));
}

void opeu_battle_apply_bombard(Game& game, uint16_t target) {
  auto&      data{ Data(game) };
  auto const damage{ opeu_battle_mine_damage(game, target) };
  data.battle_element_destroyed = 0;
  if (damage != 0) {
    auto&      hit{ Record<Element>(data, target) };
    auto const old{ hit.quantity };
    SubWord(hit.quantity, damage);
    SpendSupplies(game, old, target);
    if (hit.quantity == 0) {
      hit.damage                    = 0;
      data.battle_element_destroyed = 1;
    } else if (auto const category{ data.element_types[hit.type].category }; category != 0) {
      auto const amount{ port_rand_bounded(game, damage >> 1) };
      AddWord(hit.damage, amount, data.request_capacities[category]);
    }
  }
  opeu_battle_set_order(game, target, static_cast<uint8_t>(0));
}

void opeu_battle_resolve_attack(Game& game, uint16_t attacker, uint16_t target) {
  if (attacker != 0)
    opeu_battle_apply_damage(game, attacker, target);
  else
    opeu_battle_apply_bombard(game, target);
  if (Data(game).battle_element_destroyed != 0) opeu_battle_element_lost(game, target);
}
