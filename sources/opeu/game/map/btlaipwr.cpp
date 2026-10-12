// The computer's battle force balance and the plan its unit fights by (1..5, from cautious to bold).
#include "types.hpp"
#include "map/btlaipwr.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "map/btlaiact.hpp"

using namespace opeu::game;

namespace {
// Categories 5..7 count one each, infantry a tenth of its men plus one, the others their quantity.
constexpr uint8_t FIRST_SINGLE_CATEGORY{ 5 };

// The mean of one byte of the unit's element records (all of them, active or not).
template <typename Field>
[[nodiscard]] auto Average(DataSegment& data, uint16_t unit, Field field)
    -> uint16_t {
  uint16_t sum{ 0 };
  uint8_t  count{ 0 };
  for (auto& element : Elements(data, Record<Unit>(data, unit))) {
    sum = static_cast<uint16_t>(sum + element.*field);
    ++count;
  }
  return static_cast<uint16_t>(sum / opeu_max16(count, 1));
}
} // namespace

uint16_t opeu_battle_ai_side_power(uint8_t* ds, BattleEvent event, uint8_t side,
                                   uint16_t all) {
  auto&    data{ Data(ds) };
  uint16_t total{ 0 };
  for (uint8_t i{ 0 }; i < BATTLE_LIST_LENGTH; ++i) {
    auto const unit{ BattleList(data, side, i) };
    if (!unit)
      continue;
    for (auto& element : Elements(data, Record<Unit>(data, unit))) {
      auto const near{ Near(data, element) };
      if (!opeu_battle_element_active(ds, event, near))
        continue;
      if (!all && (!opeu_battle_element_flag2(ds, event, near) ||
                   !opeu_battle_level_allowed(ds, event, near)))
        continue;
      auto const category{ data.element_types[element.type].category };
      if (category >= FIRST_SINGLE_CATEGORY && category <= 7)
        ++total;
      else
        total =
            static_cast<uint16_t>(total + (category == Bits(Category::INFANTRY)
                                               ? element.quantity / 10 + 1
                                               : element.quantity));
    }
  }
  return opeu_max16(total, 1);
}

uint8_t opeu_battle_ai_power_ratio(uint8_t* ds, BattleEvent event,
                                   uint16_t unit) {
  auto&          data{ Data(ds) };
  auto const     side{ data.battle_ai_side };
  uint16_t const power[2]{
    opeu_battle_ai_side_power(ds, event, 0, side == 0),
    opeu_battle_ai_side_power(ds, event, 1, side != 0),
  };
  auto const other{ static_cast<uint8_t>(side == 0) };
  if (power[other] == 0)
    return 5;
  uint16_t ratio{ static_cast<uint16_t>(
      power[1] > power[0] ? power[1] / opeu_max16(power[0], 1)
                          : power[0] / opeu_max16(power[1], 1)) };
  if (ratio > 5) {
    if (auto const found{ Call(ds, event, BattleRoutine::FIND_UNIT_AT,
                               data.battle_attacker_x,
                               data.battle_attacker_y) }) {
      if (found != unit)
        return power[side & 1] < power[other] ? 1 : 5;
      ratio = 5;
    }
  }
  if (ratio < 1)
    ratio = 1;
  if (power[side & 1] > power[other]) {
    if (ratio > 2)
      return 5;
    if (ratio == 2)
      return 4;
    return 3;
  }
  if (ratio > 2)
    return unit != Near(data, data.battle_garrison) ? 1 : 2;
  if (ratio == 2)
    return 2;
  return 3;
}

uint8_t opeu_battle_ai_choose_plan(uint8_t* ds, BattleEvent event,
                                   uint16_t unit) {
  auto& data{ Data(ds) };
  int   bravery{ 0 };
  int   experience{ 0 };
  int   training{ 0 };
  int   morale{ 0 };
  auto  plan{ opeu_battle_ai_power_ratio(ds, event, unit) };
  auto  value{ Average(data, unit, &Element::morale) };
  if (value < 20)
    morale = -1;
  if (value > 60)
    morale = 1;
  if (value > 80)
    morale = 2;
  value = Average(data, unit, &Element::training);
  if (value > 60)
    training = 1;
  if (value < 10)
    training = -1;
  value = Average(data, unit, &Element::experience);
  if (value > 60)
    experience = -1;
  if (value > 80)
    experience = -2;
  uint16_t commander;
  if (unit == Near(data, data.battle_garrison))
    commander = Call(ds, event, BattleRoutine::FIND_SIDE_RECORD,
                     opeu_battle_unit_side(ds, event, unit), 1);
  else {
    auto const group{ static_cast<uint8_t>(
        Call(ds, event, BattleRoutine::UNIT_GROUP_INDEX, unit)) };
    commander = Call(ds, event, BattleRoutine::FIND_GROUP_RECORD,
                     opeu_battle_unit_side(ds, event, unit), group);
  }
  if (commander &&
      !(Record<Person>(data, commander).role & Bits(PersonRole::DEPUTY))) {
    auto const courage{ Record<Person>(data, commander).bravery };
    if (courage < 30)
      bravery = -1;
    if (courage < 10)
      bravery = -2;
    if (courage > 70)
      bravery = 1;
    if (courage > 80)
      bravery = 2;
    if (courage > 90)
      bravery = 3;
  }
  plan = static_cast<uint8_t>(plan + bravery + experience + training + morale);
  return static_cast<uint8_t>(opeu_min16(opeu_max16(plan, 1), 5));
}

uint16_t svc_btlaipwr(uint8_t* ds, uint32_t service, const uint16_t* args,
                      unsigned int count) {
  return svc_battle_ai(ds, service, args, count);
}
