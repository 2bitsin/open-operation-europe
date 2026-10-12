// A battle fought out of sight: its run loop, the elements' battle records, the damage to the objective, and the
// entry.
#include "map/btlend.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/numeric.hpp"
#include "core/recindex.hpp"
#include "map/btlfight.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t ROUNDS{ 20 };
constexpr uint8_t NO_DIRECTION{ 0xff };
} // namespace

// Up to 20 rounds; the battle ends early when the computer turn decides it or a side gives way.
void opeu_battle_run(uint8_t* ds, BattleEvent event) {
  auto& data{ Data(ds) };
  for (data.battle_level_limit = 0; data.battle_level_limit < ROUNDS; ++data.battle_level_limit) {
    Call(ds, event, BattleRoutine::BATTLE_MORALE_CHECK);
    if (static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_COMPUTER_TURN)) || !opeu_battle_rounds(ds, event)) {
      opeu_battle_finish_turn(ds, event);
      return;
    }
  }
  opeu_battle_finish_none(ds, event);
}

// Each element's rounds between shots come from its strength, and the enemy sees it.
void opeu_battle_refresh_strengths(uint8_t* ds, BattleEvent event) {
  auto& data{ Data(ds) };
  for (unsigned side{ 0 }; side < SIDE_COUNT; ++side)
    for (unsigned i{ 0 }; i < BATTLE_LIST_LENGTH; ++i)
      if (auto const unit{ BattleList(data, side, i) })
        for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) {
          auto const rounds{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_STRENGTH, it.Near())) };
          PositionOf(data, it.Near()).rounds  = rounds;
          (*it).flags                        |= Bits(ElementFlag::SPOTTED);
        }
}

// The element's battle record is the index-th: off the field, at rest, with no goal.
void opeu_battle_init_element(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t side, uint8_t index) {
  auto& data{ Data(ds) };
  auto& record{ Record<Element>(data, element) };
  if (!side)
    record.flags &= static_cast<uint8_t>(~Bits(ElementFlag::SIDE_1));
  else
    record.flags |= Bits(ElementFlag::SIDE_1);
  record.position = static_cast<uint16_t>(Near(data, data.battle_positions) + index * sizeof(BattlePosition));
  auto& position{ PositionOf(data, element) };
  position.x      = 0;
  position.y      = BATTLE_UNPLACED;
  position.state  = 1;
  position.goal_x = 0;
  position.goal_y = 0;
  Call(ds, event, BattleRoutine::BATTLE_SET_ORDER, 0, element);
}

// The garrison fights for the objective's side, every other unit for its own.
void opeu_battle_init_elements(uint8_t* ds, BattleEvent event) {
  auto&   data{ Data(ds) };
  uint8_t index{ 0 };
  for (unsigned side{ 0 }; side < SIDE_COUNT; ++side)
    for (unsigned i{ 0 }; i < BATTLE_LIST_LENGTH; ++i) {
      auto const unit{ BattleList(data, side, i) };
      if (unit == 0) continue;
      if (unit == Near(data, data.battle_garrison)) {
        auto const site{ opeu_find_objective_at(ds, data.battle_x, data.battle_y) };
        for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) {
          auto const owner{ opeu_objective_side(ds, site) };
          opeu_battle_init_element(ds, event, it.Near(), owner, index++);
        }
      } else {
        auto const team{ opeu_unit_side(unit) };
        for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) opeu_battle_init_element(ds, event, it.Near(), team, index++);
      }
    }
}

// In two battles of five the objective loses strength. The counts are those of each side's last unit; the original's
// counters are uninitialised when a list is empty and start at zero here.
void opeu_battle_objective_damage(uint8_t* ds, BattleEvent event) {
  auto&      data{ Data(ds) };
  auto const site{ opeu_find_objective_at(ds, data.battle_x, data.battle_y) };
  if (site == 0) return;
  if (Call(ds, event, BattleRoutine::RAND_BOUNDED, 100) > 40) return;
  uint8_t ours{ 0 };
  uint8_t enemies{ 0 };
  for (unsigned i{ 0 }; BattleList(data, 0, i) != 0; ++i) ours = static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_COUNT, BattleList(data, 0, i)));
  for (unsigned i{ 0 }; BattleList(data, 1, i) != 0; ++i) enemies = static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_COUNT, BattleList(data, 1, i)));
  auto loss{ static_cast<uint16_t>(ours - enemies) };
  if (loss < 0xfffc) return;
  if (loss < 1) {
    auto const r{ static_cast<uint16_t>(Call(ds, event, BattleRoutine::RAND_BOUNDED, 51) + 50) };
    loss = opeu_mul_div(static_cast<uint16_t>(loss + 5), r, 100);
  }
  if (loss > 7) {
    auto const r{ static_cast<uint16_t>(Call(ds, event, BattleRoutine::RAND_BOUNDED, 101) + 50) };
    loss = opeu_mul_div(7, r, 100);
  }
  opeu_sub_byte(static_cast<uint8_t>(loss), &Record<Objective>(data, site).strength);
}

void opeu_notice_other(uint8_t* ds, BattleEvent event) {
  Data(ds).battle_direction = NO_DIRECTION;
  opeu_battle_init_elements(ds, event);
  opeu_battle_refresh_strengths(ds, event);
  opeu_battle_objective_damage(ds, event);
  Call(ds, event, BattleRoutine::BATTLE_TIRE_LEADERS);
  opeu_battle_run(ds, event);
}

uint16_t svc_btlend(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}
