// The player's battle orders for one element: against an element or toward the battle cursor's cell, and the notice
// when an order is refused.
#include "map/btlcmds.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcmdr.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {
constexpr uint8_t NONE{ 0xff };
// opeu_battle_order_refused's kinds.
constexpr uint8_t REFUSED_MOVE{ 1 };
constexpr uint8_t REFUSED_TARGET{ 2 };

void TargetOrder(Game& game, uint16_t element, uint16_t target, BattleState state) {
  auto& position{ PositionOf(Data(game), element) };
  position.state  = Bits(state);
  position.target = target;
  opeu_battle_engage(game, element);
  opeu_battle_set_facing(game, element, NONE);
  opeu_battle_set_order(game, element, 0);
}

void CellOrder(Game& game, uint16_t element, uint8_t x, uint8_t y, BattleState state) {
  auto& position{ PositionOf(Data(game), element) };
  position.state  = Bits(state);
  position.goal_x = x;
  position.goal_y = y;
  opeu_battle_disengage(game, element);
  opeu_battle_set_facing(game, element, NONE);
  opeu_battle_set_order(game, element, 0);
}

void CursorOrder(Game& game, uint16_t element, BattleState state) {
  auto const& data{ Data(game) };
  CellOrder(game, element, data.battle_cursor_x, data.battle_cursor_y, state);
}

// The order to stay: the state is stored before the element's cell is read, as in the original.
void StayOrder(Game& game, uint16_t element, BattleState state) {
  auto& position{ PositionOf(Data(game), element) };
  position.state = Bits(state);
  CellOrder(game, element, position.x, position.y, state);
}
} // namespace

void opeu_battle_order_attack(Game& game, uint16_t element, uint16_t target) {
  TargetOrder(game, element, target, BattleState::ATTACK);
}

void opeu_battle_order_attack_cell(Game& game, uint16_t element) {
  CursorOrder(game, element, BattleState::ATTACK);
}

void opeu_battle_order_move(Game& game, uint16_t element) {
  CursorOrder(game, element, BattleState::MOVE);
}

void opeu_battle_order_blitz(Game& game, uint16_t element, uint16_t target) {
  TargetOrder(game, element, target, BattleState::BLITZ);
}

void opeu_battle_order_blitz_cell(Game& game, uint16_t element) {
  CursorOrder(game, element, BattleState::BLITZ);
}

void opeu_battle_order_withdraw(Game& game, uint16_t element) {
  CursorOrder(game, element, BattleState::WITHDRAW);
}

void opeu_battle_order_defend(Game& game, uint16_t element) {
  StayOrder(game, element, BattleState::DEFEND);
}

void opeu_battle_order_rest(Game& game, uint16_t element) {
  StayOrder(game, element, BattleState::REST);
}

void opeu_battle_order_refused(Game& game, uint8_t kind) {
  auto& data{ Data(game) };
  if (kind != REFUSED_MOVE && kind != REFUSED_TARGET) return;
  opeu_battle_commander_message(game, kind == REFUSED_MOVE ? Near(data, data.battle_msg_cannot_move) : Near(data, data.battle_msg_select_target), static_cast<uint8_t>(data.battle_active_side),
                                static_cast<uint8_t>(1));
}
