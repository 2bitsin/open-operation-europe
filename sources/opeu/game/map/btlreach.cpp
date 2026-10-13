// The player's battle exit cells and the lone withdrawal check.
//
// mark_exits frames every cell the element can enter and leave the battle from (and reports whether there is one),
// unmark_exits redraws those cells, and lone_withdrawal reports whether the element is the only one of its unit
// withdrawing, refusing once the element has left the field.
#include "map/btlreach.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcmds.hpp"
#include "map/btldraw.hpp"
#include "map/btlexit.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
[[nodiscard]] auto ExitCell(Game& game, BattleEvent event, uint8_t x, uint8_t y, uint16_t element) -> bool {
  return Call(game, event, BattleRoutine::BATTLE_CAN_ENTER, x, y, element) && opeu_battle_exit_ok(game, event, x, y, element);
}
} // namespace

uint16_t opeu_battle_mark_exits(Game& game, BattleEvent event, uint16_t element) {
  uint8_t count{ 0 };
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x)
      if (ExitCell(game, event, x, y, element)) {
        ++count;
        opeu_battle_draw_frame(game, event, x, y);
      }
  return count != 0;
}

void opeu_battle_unmark_exits(Game& game, BattleEvent event, uint16_t element) {
  for (uint8_t y{ 0 }; y < BATTLE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < BATTLE_WIDTH; ++x)
      if (ExitCell(game, event, x, y, element)) Call(game, event, BattleRoutine::BATTLE_REFRESH_CELL, x, y);
}

uint16_t opeu_battle_lone_withdrawal(Game& game, BattleEvent event, uint16_t element) {
  auto&      data{ Data(game) };
  uint16_t   last{ 0 };
  uint8_t    count{ 0 };
  auto const unit{ opeu_battle_element_unit(game, event, element) };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) {
    if (PositionOf(data, element).x & BATTLE_LEFT) return 0;
    if (opeu_battle_state(game, event, it.Near()) == std::to_underlying(BattleState::WITHDRAW)) {
      ++count;
      last = it.Near();
    }
  }
  return count == 1 && last == element;
}

// The player orders segment's services (map/btlcmds).
uint16_t svc_btlreach(Game& game, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_orders(game, service, args, count);
}
