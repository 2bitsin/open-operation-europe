// The battle set-up and round loop run by opeu_notice_one.
//
// opeu_battle_reset_elements gives every element of the engaged units a fresh position record and its side bit;
// opeu_battle_tire_leaders takes health from each engaged group's general (or the side's commander) and
// opeu_battle_run_rounds runs up to ten rounds.
#include "types.hpp"
#include "map/btlsetup.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/memory.hpp"
#include "map/btllist.hpp"
#include "map/btlround.hpp"
#include "map/btlunit.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {
constexpr uint8_t  ROUND_COUNT{ 10 };
constexpr uint8_t  GROUP_PRESENCE{ SIDE_COUNT * GROUPS_PER_SIDE };
constexpr uint16_t BANNER_X{ 2 };
constexpr uint16_t BANNER_Y{ 0x90 };
constexpr uint16_t BATTLE_MUSIC_PLAYER{ 7 };
constexpr uint16_t BATTLE_MUSIC_COMPUTER{ 8 };
constexpr uint8_t  POSITION_BYTES{ sizeof(BattlePosition) };

// A general's health drops by 3 at night, else by 2.
void Tire(uint8_t* ds, BattleEvent event, uint16_t person) {
  auto&      data{ Data(ds) };
  auto const night{ Call(ds, event, BattleRoutine::HOUR_IS_NIGHT, data.hour) };
  Call(ds, event, BattleRoutine::SUB_BYTE, night ? 3 : 2,
       Near(data, Record<Person>(data, person).health));
}
} // namespace

uint16_t opeu_battle_side_needs_input(uint8_t* ds, BattleEvent event,
                                      uint8_t side) {
  auto& data{ Data(ds) };
  for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
    if (auto const unit{ BattleList(data, side, slot) };
        unit && !Call(ds, event, BattleRoutine::BATTLE_UNIT_CONTROLLED, unit))
      return 1;
  return 0;
}

void opeu_battle_run_rounds(uint8_t* ds, BattleEvent event) {
  auto& data{ Data(ds) };
  for (data.battle_level_limit = 0; data.battle_level_limit < ROUND_COUNT;
       ++data.battle_level_limit) {
    if (static_cast<uint8_t>(
            Call(ds, event, BattleRoutine::BATTLE_COMPUTER_TURN))) {
      opeu_battle_report_outcome(ds, event);
      return;
    }
    for (uint8_t line{ 1 }; line <= 4; ++line)
      Call(ds, event, BattleRoutine::BATTLE_RESTORE_TILE, line, 9);
    Call(ds, event, BattleRoutine::ARMY_POSITION, BANNER_X, BANNER_Y);
    Call(ds, event, BattleRoutine::CURSOR_PRINT,
         Near(data, data.battle_round_format), data.battle_level_limit + 1);
    if (opeu_battle_side_needs_input(ds, event, 0) ||
        opeu_battle_side_needs_input(ds, event, 1))
      Call(ds, event, BattleRoutine::BATTLE_SELECT_SERVICE);
    if (data.battle_level_limit)
      Call(ds, event, BattleRoutine::BATTLE_ANNOUNCE_READY);
    if (!Call(ds, event, BattleRoutine::BATTLE_TURN_LOOP)) {
      opeu_battle_report_outcome(ds, event);
      return;
    }
  }
  opeu_battle_report_stalemate(ds, event);
}

void opeu_battle_tire_leaders(uint8_t* ds, BattleEvent event) {
  auto&                            data{ Data(ds) };
  std::array<bool, GROUP_PRESENCE> present{};
  for (uint8_t side{ 0 }; side < SIDE_COUNT; ++side)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot)
      if (auto const unit{ BattleList(data, side, slot) };
          unit && unit != GARRISON_AT) {
        unsigned at{ opeu_battle_unit_side(ds, event, unit) * GROUPS_PER_SIDE };
        at += static_cast<uint8_t>(
            Call(ds, event, BattleRoutine::UNIT_GROUP_INDEX, unit));
        if (at < GROUP_PRESENCE)
          present[at] = true;
      }
  for (uint8_t side{ 0 }; side < SIDE_COUNT; ++side) {
    if (!data.battle_command[side]) {
      for (uint8_t group{ 0 }; group < GROUPS_PER_SIDE; ++group)
        if (present[side * GROUPS_PER_SIDE + group])
          Tire(ds, event,
               Call(ds, event, BattleRoutine::FIND_GROUP_RECORD, side, group));
    } else {
      Tire(ds, event,
           Call(ds, event, BattleRoutine::FIND_SIDE_RECORD, side, 1));
    }
  }
}

void opeu_battle_start_music(uint8_t* ds, BattleEvent event) {
  auto&      data{ Data(ds) };
  auto const side{ opeu_battle_unit_side(ds, event, data.battle_lists[0]) };
  Call(ds, event, BattleRoutine::SET_TURN_CONTEXT,
       data.side_status[side] == SIDE_PLAYED ? BATTLE_MUSIC_PLAYER
                                             : BATTLE_MUSIC_COMPUTER);
}

// side: nonzero for side 1. index: the element's position record.
void opeu_battle_reset_element(uint8_t* ds, BattleEvent event, uint16_t element,
                               uint8_t side, uint8_t index) {
  auto& data{ Data(ds) };
  auto& record{ Record<Element>(data, element) };
  if (!side)
    record.flags &= static_cast<uint8_t>(~Bits(ElementFlag::SIDE_1));
  else
    record.flags |= Bits(ElementFlag::SIDE_1);
  record.position = static_cast<uint16_t>(Near(data, data.battle_positions) +
                                          index * POSITION_BYTES);
  auto& position{ PositionOf(data, element) };
  position.x         = BATTLE_UNPLACED;
  position.y         = BATTLE_UNPLACED;
  position.facing    = 0xff;
  position.unknown_3 = 0xff;
  position.strength  = 0xff;
  position.state     = std::to_underlying(BattleState::DEFEND);
  position.goal_x    = 0;
  position.goal_y    = 0;
  opeu_battle_set_order(ds, event, element, 0);
}

void opeu_battle_reset_elements(uint8_t* ds, BattleEvent event) {
  auto&   data{ Data(ds) };
  uint8_t index{ 0 };
  for (uint8_t side{ 0 }; side < SIDE_COUNT; ++side)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot) {
      auto const unit{ BattleList(data, side, slot) };
      if (!unit)
        continue;
      uint8_t element_side;
      if (unit == GARRISON_AT) {
        auto const objective{ Call(ds, event, BattleRoutine::FIND_OBJECTIVE_AT,
                                   data.battle_x, data.battle_y) };
        element_side = (Record<Objective>(data, objective).flags &
                        Bits(ObjectiveFlag::SIDE_1)) != 0;
      } else {
        element_side = static_cast<uint8_t>(
            Call(ds, event, BattleRoutine::UNIT_SIDE, unit));
      }
      for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() };
           it != std::default_sentinel; ++it)
        opeu_battle_reset_element(ds, event, it.Near(), element_side, index++);
    }
}

void opeu_notice_one(uint8_t* ds, BattleEvent event) {
  opeu_battle_reset_elements(ds, event);
  opeu_battle_start_music(ds, event);
  Call(ds, event, BattleRoutine::BATTLE_MAP_SETUP);
  Call(ds, event, BattleRoutine::DEPLOY_BATTLE);
  opeu_battle_tire_leaders(ds, event);
  opeu_battle_run_rounds(ds, event);
  Call(ds, event, BattleRoutine::VGA_CLEAR);
  Call(ds, event, BattleRoutine::PALETTE_FOR_HOUR);
  Call(ds, event, BattleRoutine::SCREEN_RESET);
  Call(ds, event, BattleRoutine::DAILY_CONTEXT, Data(ds).current_side);
}

uint16_t svc_btlsetup(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_tire_leaders(std::uint8_t* ds,
                                           std::uint16_t const*, unsigned)
    -> std::uint32_t {
  opeu_battle_tire_leaders(ds, svc_btlsetup);
  return 0;
}
