// A player's battle order command for an element. Commands 0..5 order the element alone, 6..11 every element of
// its unit still on the field: 0/6 move, 1/7 attack, 2/8 blitz, 5/11 withdraw, 3/4/9/10 nothing. The destination is
// picked with the battle cursor; an invalid pick shows a message and prompts again.
#include "map/btlexec.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcmds.hpp"
#include "map/btlcurs.hpp"
#include "map/btlexit.hpp"
#include "map/btllist.hpp"
#include "map/btlsel.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// What the command orders each element to do.
enum class Kind : uint8_t {
  MOVE,
  ATTACK,
  ATTACK_CELL,
  BLITZ,
  BLITZ_CELL,
  WITHDRAW
};
// opeu_battle_order_refused reasons.
constexpr uint8_t REFUSED_TERRAIN{ 1 };
constexpr uint8_t REFUSED_FRIEND{ 2 };
constexpr uint8_t IMPASSABLE{ 0xff }; // a battle_terrain_costs entry
// Added to a command for every element of the unit.
constexpr uint8_t WHOLE_UNIT{ 6 };
// Below the frame: the stack the cell selection is entered with.
constexpr uint16_t SELECT_STACK{ 0x18 };

void Message(uint8_t* ds, BattleEvent event, uint16_t text) {
  Call(ds, event, BattleRoutine::BATTLE_COMMANDER_MESSAGE, Data(ds).battle_active_side, 1, text);
}

// The cost for the element to enter the cursor's cell.
[[nodiscard]] auto TerrainCost(uint8_t* ds, BattleEvent event, uint16_t element) -> uint8_t {
  auto&      data{ Data(ds) };
  auto const terrain{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_CELL_TERRAIN, data.battle_cursor_x, data.battle_cursor_y)) };
  auto const category{ data.element_types[Record<Element>(data, element).type].category };
  return data.battle_terrain_costs[terrain * 8u + category];
}

void OrderOne(uint8_t* ds, BattleEvent event, Kind kind, uint16_t element, uint16_t target) {
  switch (kind) {
  case Kind::MOVE:        opeu_battle_order_move(ds, event, element); break;
  case Kind::ATTACK:      opeu_battle_order_attack(ds, event, element, target); break;
  case Kind::ATTACK_CELL: opeu_battle_order_attack_cell(ds, event, element); break;
  case Kind::BLITZ:       opeu_battle_order_blitz(ds, event, element, target); break;
  case Kind::BLITZ_CELL:  opeu_battle_order_blitz_cell(ds, event, element); break;
  default:                opeu_battle_order_withdraw(ds, event, element); break;
  }
}

void OrderAll(uint8_t* ds, BattleEvent event, Kind kind, bool whole, uint16_t element, uint16_t unit, uint16_t target) {
  if (!whole) {
    OrderOne(ds, event, kind, element, target);
    return;
  }
  auto& data{ Data(ds) };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
    if (!(PositionOf(data, it.Near()).x & BATTLE_LEFT)) OrderOne(ds, event, kind, it.Near(), target);
}
} // namespace

uint16_t opeu_battle_order_command(uint8_t* ds, BattleEvent event, uint16_t element, uint8_t command, uint16_t frame) {
  auto&      data{ Data(ds) };
  auto const whole{ command >= WHOLE_UNIT };
  auto const side{ [&](uint16_t e) { return opeu_battle_element_side(ds, event, e); } };
  auto const target_at_cursor{ [&] { return Call(ds, event, BattleRoutine::BATTLE_ELEMENT_AT, data.battle_cursor_x, data.battle_cursor_y); } };
  for (;;) {
    auto const unit{ opeu_battle_element_unit(ds, event, element) };
    Call(ds, event, BattleRoutine::INPUT_FLUSH);
    Call(ds, event, BattleRoutine::BATTLE_COMMANDER_PANEL, data.battle_active_side, 0);
    Call(ds, event, BattleRoutine::UI_TEXT, Near(data, data.battle_msg_set_target));
    if (!opeu_battle_select_cell(ds, event, Local(frame, SELECT_STACK))) return 0;
    switch (command) {
    case 1:
    case 7: {
      auto const target{ target_at_cursor() };
      if (target && opeu_battle_element_flag2(ds, event, target)) {
        if (side(target) != side(element)) OrderAll(ds, event, Kind::ATTACK, whole, element, unit, target);
        return 1;
      }
      if (target && side(element) == side(target)) {
        opeu_battle_order_refused(ds, event, REFUSED_FRIEND);
        continue;
      }
      if ((BattleTerrainAt(data, data.battle_cursor_x, data.battle_cursor_y) & BATTLE_TERRAIN_MASK) == BATTLE_CITY && side(element) == opeu_battle_unit_side(ds, event, data.battle_lists[0])) {
        OrderAll(ds, event, Kind::ATTACK_CELL, whole, element, unit, 0);
        return 1;
      }
      Message(ds, event, Near(data, data.battle_msg_no_units));
      continue;
    }
    case 0:
    case 6:
      if (TerrainCost(ds, event, element) != IMPASSABLE && opeu_battle_cell_open(ds, event, data.battle_cursor_x, data.battle_cursor_y, element)) {
        OrderAll(ds, event, Kind::MOVE, whole, element, unit, 0);
        return 1;
      }
      opeu_battle_order_refused(ds, event, REFUSED_TERRAIN);
      continue;
    case 2:
    case 8: {
      auto const target{ target_at_cursor() };
      if (target && opeu_battle_element_flag2(ds, event, target)) {
        if (side(target) != side(element)) {
          OrderAll(ds, event, Kind::BLITZ, whole, element, unit, target);
          return 1;
        }
      } else if (target && side(element) == side(target)) {
        opeu_battle_order_refused(ds, event, REFUSED_FRIEND);
        continue;
      }
      if (TerrainCost(ds, event, element) == IMPASSABLE) {
        opeu_battle_order_refused(ds, event, REFUSED_TERRAIN);
        continue;
      }
      OrderAll(ds, event, Kind::BLITZ_CELL, whole, element, unit, 0);
      return 1;
    }
    case 5:
    case 11:
      if (opeu_battle_cursor_exit_ok(ds, event, element)) {
        OrderAll(ds, event, Kind::WITHDRAW, whole, element, unit, 0);
        return 1;
      }
      Message(ds, event, Near(data, data.battle_msg_withdraw_from));
      continue;
    default: return 1;
    }
  }
}

// The services of this module: the player orders group's (map/btlcmds).
uint16_t svc_btlexec(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_orders(ds, service, args, count);
}
