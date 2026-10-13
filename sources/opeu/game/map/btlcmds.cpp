// The player's battle orders for one element: against an element or toward the battle cursor's cell, and the notice
// when an order is refused.
#include "map/btlcmds.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

#include <utility>

using namespace opeu::game;

namespace {
constexpr uint8_t NONE{ 0xff };
// opeu_battle_order_refused's kinds.
constexpr uint8_t  REFUSED_MOVE{ 1 };
constexpr uint8_t  REFUSED_TARGET{ 2 };
constexpr uint16_t HEX_MASK_BYTES{ 390 };

void TargetOrder(Game& game, BattleEvent event, uint16_t element, uint16_t target, BattleState state) {
  auto& position{ PositionOf(Data(game), element) };
  position.state  = Bits(state);
  position.target = target;
  opeu_battle_engage(game, event, element);
  opeu_battle_set_facing(game, event, element, NONE);
  opeu_battle_set_order(game, event, element, 0);
}

void CellOrder(Game& game, BattleEvent event, uint16_t element, uint8_t x, uint8_t y, BattleState state) {
  auto& position{ PositionOf(Data(game), element) };
  position.state  = Bits(state);
  position.goal_x = x;
  position.goal_y = y;
  opeu_battle_disengage(game, event, element);
  opeu_battle_set_facing(game, event, element, NONE);
  opeu_battle_set_order(game, event, element, 0);
}

void CursorOrder(Game& game, BattleEvent event, uint16_t element, BattleState state) {
  auto const& data{ Data(game) };
  CellOrder(game, event, element, data.battle_cursor_x, data.battle_cursor_y, state);
}

// The order to stay: the state is stored before the element's cell is read, as in the original.
void StayOrder(Game& game, BattleEvent event, uint16_t element, BattleState state) {
  auto& position{ PositionOf(Data(game), element) };
  position.state = Bits(state);
  CellOrder(game, event, element, position.x, position.y, state);
}
} // namespace

void opeu_battle_order_attack(Game& game, BattleEvent event, uint16_t element, uint16_t target) {
  TargetOrder(game, event, element, target, BattleState::ATTACK);
}

void opeu_battle_order_attack_cell(Game& game, BattleEvent event, uint16_t element) {
  CursorOrder(game, event, element, BattleState::ATTACK);
}

void opeu_battle_order_move(Game& game, BattleEvent event, uint16_t element) {
  CursorOrder(game, event, element, BattleState::MOVE);
}

void opeu_battle_order_blitz(Game& game, BattleEvent event, uint16_t element, uint16_t target) {
  TargetOrder(game, event, element, target, BattleState::BLITZ);
}

void opeu_battle_order_blitz_cell(Game& game, BattleEvent event, uint16_t element) {
  CursorOrder(game, event, element, BattleState::BLITZ);
}

void opeu_battle_order_withdraw(Game& game, BattleEvent event, uint16_t element) {
  CursorOrder(game, event, element, BattleState::WITHDRAW);
}

void opeu_battle_order_defend(Game& game, BattleEvent event, uint16_t element) {
  StayOrder(game, event, element, BattleState::DEFEND);
}

void opeu_battle_order_rest(Game& game, BattleEvent event, uint16_t element) {
  StayOrder(game, event, element, BattleState::REST);
}

void opeu_battle_order_refused(Game& game, BattleEvent event, uint8_t kind) {
  auto& data{ Data(game) };
  if (kind != REFUSED_MOVE && kind != REFUSED_TARGET) return;
  Call(game, event, BattleRoutine::BATTLE_COMMANDER_MESSAGE, data.battle_active_side, 1, kind == REFUSED_MOVE ? Near(data, data.battle_msg_cannot_move) : Near(data, data.battle_msg_select_target));
}

// The services of the player orders segment; the hex mask table is read from its far segment.
uint16_t svc_battle_orders(Game& game, uint32_t address, const uint16_t* args, unsigned count) {
  auto const masks{ std::to_underlying(BattleRoutine::BATTLE_HEX_MASKS) };
  if (address >= masks && address < masks + HEX_MASK_BYTES) return *port_far(0x352du, static_cast<uint16_t>(address - masks));
  return port_service(game, address, args, count);
}

uint16_t svc_btlcmds(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_orders(game, service, args, count);
}
