// The player's battle exit cells and the lone withdrawal check.
//
// mark_exits frames every cell the element can enter and leave the battle from (and reports whether there is one),
// unmark_exits redraws those cells, and lone_withdrawal reports whether the element is the only one of its unit
// withdrawing, refusing once the element has left the field.
#include "map/btlreach.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcmds.hpp"
#include "map/btldraw.hpp"
#include "map/btlelem.hpp"
#include "map/btlexit.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

#include <ranges>

using namespace opeu::game;

namespace {
[[nodiscard]] auto ExitCell(Game& game, uint8_t x, uint8_t y, uint16_t element) -> bool {
  return opeu_battle_can_enter(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), element) && opeu_battle_exit_ok(game, GridCell{ x, y }, element);
}
} // namespace

uint16_t opeu_battle_mark_exits(Game& game, uint16_t element) {
  uint8_t count{ 0 };
  for (auto const y : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH))
    for (auto const x : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH))
      if (ExitCell(game, x, y, element)) {
        ++count;
        opeu_battle_draw_frame(game, x, y);
      }
  return count != 0;
}

void opeu_battle_unmark_exits(Game& game, uint16_t element) {
  for (auto const y : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH))
    for (auto const x : std::views::iota(uint8_t{ 0 }, BATTLE_WIDTH))
      if (ExitCell(game, x, y, element)) opeu_battle_refresh_cell(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y));
}

uint16_t opeu_battle_lone_withdrawal(Game& game, uint16_t element) {
  auto&      data{ Data(game) };
  uint16_t   last{ 0 };
  uint8_t    count{ 0 };
  auto const unit{ opeu_battle_element_unit(game, element) };
  for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it) {
    if (PositionOf(data, element).x & BATTLE_LEFT) return 0;
    if (opeu_battle_state(game, it.Near()) == std::to_underlying(BattleState::WITHDRAW)) {
      ++count;
      last = it.Near();
    }
  }
  return count == 1 && last == element;
}
