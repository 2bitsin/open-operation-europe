// A player's battle order command for an element. Commands 0..5 order the element alone, 6..11 every element of
// its unit still on the field: 0/6 move, 1/7 attack, 2/8 blitz, 5/11 withdraw, 3/4/9/10 nothing. The destination is
// picked with the battle cursor; an invalid pick shows a message and prompts again.
#include "map/btlexec.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcmdr.hpp"
#include "map/btlcmds.hpp"
#include "map/btlcurs.hpp"
#include "map/btlexit.hpp"
#include "map/btllist.hpp"
#include "map/btlsel.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
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
// Added to a command for every element of the unit.
constexpr uint8_t WHOLE_UNIT{ 6 };

void Message(Game& game, uint16_t text) {
  opeu_battle_commander_message(game, text, static_cast<uint8_t>(Data(game).battle_active_side), static_cast<uint8_t>(1));
}

// The cost for the element to enter the cursor's cell.
[[nodiscard]] auto TerrainCost(Game& game, uint16_t element) -> uint8_t {
  auto&      data{ Data(game) };
  auto const terrain{ static_cast<uint8_t>(opeu_battle_cell_terrain(game, static_cast<uint8_t>(data.battle_cursor_x), static_cast<uint8_t>(data.battle_cursor_y))) };
  auto const category{ ElementTypeOf(data, element).category };
  return data.battle_terrain_costs[terrain * 8u + category];
}

void OrderOne(Game& game, Kind kind, uint16_t element, uint16_t target) {
  switch (kind) {
  case Kind::MOVE:        opeu_battle_order_move(game, element); break;
  case Kind::ATTACK:      opeu_battle_order_attack(game, element, target); break;
  case Kind::ATTACK_CELL: opeu_battle_order_attack_cell(game, element); break;
  case Kind::BLITZ:       opeu_battle_order_blitz(game, element, target); break;
  case Kind::BLITZ_CELL:  opeu_battle_order_blitz_cell(game, element); break;
  default:                opeu_battle_order_withdraw(game, element); break;
  }
}

void OrderAll(Game& game, Kind kind, bool whole, uint16_t element, uint16_t unit, uint16_t target) {
  if (!whole) {
    OrderOne(game, kind, element, target);
    return;
  }
  auto& data{ Data(game) };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it)
    if (!(PositionOf(data, it.Near()).x & BATTLE_LEFT)) OrderOne(game, kind, it.Near(), target);
}
} // namespace

uint16_t opeu_battle_order_command(Game& game, uint16_t element, uint8_t command) {
  auto&      data{ Data(game) };
  auto const whole{ command >= WHOLE_UNIT };
  auto const side{ [&](uint16_t e) { return opeu_battle_element_side(game, e); } };
  auto const target_at_cursor{ [&] { return opeu_battle_element_at(game, static_cast<uint8_t>(data.battle_cursor_x), static_cast<uint8_t>(data.battle_cursor_y)); } };
  for (;;) {
    auto const unit{ opeu_battle_element_unit(game, element) };
    opeu_input_flush(game, static_cast<TextHost*>(port_textout_host()));
    opeu_battle_commander_panel(game, static_cast<uint8_t>(data.battle_active_side), 0);
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.battle_msg_set_target));
    if (!opeu_battle_select_cell(game)) return 0;
    switch (command) {
    case 1:
    case 7: {
      auto const target{ target_at_cursor() };
      if (target && opeu_battle_element_flag2(game, target)) {
        if (side(target) != side(element)) OrderAll(game, Kind::ATTACK, whole, element, unit, target);
        return 1;
      }
      if (target && side(element) == side(target)) {
        opeu_battle_order_refused(game, REFUSED_FRIEND);
        continue;
      }
      if ((BattleTerrainAt(data, data.battle_cursor_x, data.battle_cursor_y) & BATTLE_TERRAIN_MASK) == BATTLE_CITY && side(element) == opeu_battle_unit_side(game, data.battle_lists[0])) {
        OrderAll(game, Kind::ATTACK_CELL, whole, element, unit, 0);
        return 1;
      }
      Message(game, Near(data, data.battle_msg_no_units));
      continue;
    }
    case 0:
    case 6:
      if (TerrainCost(game, element) != IMPASSABLE_COST && opeu_battle_cell_open(game, data.battle_cursor_x, data.battle_cursor_y, element)) {
        OrderAll(game, Kind::MOVE, whole, element, unit, 0);
        return 1;
      }
      opeu_battle_order_refused(game, REFUSED_TERRAIN);
      continue;
    case 2:
    case 8: {
      auto const target{ target_at_cursor() };
      if (target && opeu_battle_element_flag2(game, target)) {
        if (side(target) != side(element)) {
          OrderAll(game, Kind::BLITZ, whole, element, unit, target);
          return 1;
        }
      } else if (target && side(element) == side(target)) {
        opeu_battle_order_refused(game, REFUSED_FRIEND);
        continue;
      }
      if (TerrainCost(game, element) == IMPASSABLE_COST) {
        opeu_battle_order_refused(game, REFUSED_TERRAIN);
        continue;
      }
      OrderAll(game, Kind::BLITZ_CELL, whole, element, unit, 0);
      return 1;
    }
    case 5:
    case 11:
      if (opeu_battle_cursor_exit_ok(game, element)) {
        OrderAll(game, Kind::WITHDRAW, whole, element, unit, 0);
        return 1;
      }
      Message(game, Near(data, data.battle_msg_withdraw_from));
      continue;
    default: return 1;
    }
  }
}
