// The round's "ready for battle" announcements.
//
// Every engaged unit on a player's side whose level gap equals the round is announced ("<unit> is ready for
// battle!"), then the game pauses. Returns 1 when anything was announced.
#include "map/btlready.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// The DS reserve of the unit name buffer the name service writes.
constexpr uint16_t NAME_BYTES{ 0x2c };
// The text columns of the two sides' name panels and the rows of the two lines.
constexpr uint16_t SIDE_0_COLUMN{ 1 };
constexpr uint16_t SIDE_1_COLUMN{ 0x38 };
constexpr uint16_t NAME_ROW{ 0x156 };
constexpr uint16_t READY_ROW{ 0x168 };
constexpr uint16_t PAUSE{ 3 };
} // namespace

uint16_t opeu_battle_announce_ready(Game& game, BattleEvent event) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ NAME_BYTES };
  bool                announced{ false };
  for (uint8_t list{ 0 }; list < SIDE_COUNT; ++list)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot) {
      auto const unit{ BattleList(data, list, slot) };
      if (!unit) continue;
      auto const side{ opeu_battle_unit_side(game, event, unit) };
      if (data.side_status[side] != SIDE_PLAYED) continue;
      if (opeu_battle_level_gap(game, event, Record<Unit>(data, unit).first) != data.battle_level_limit) continue;
      uint16_t column;
      if (!side) {
        Call(game, event, BattleRoutine::BATTLE_NAME_CLEAR_LEFT);
        column = SIDE_0_COLUMN;
      } else {
        Call(game, event, BattleRoutine::BATTLE_NAME_CLEAR_RIGHT);
        column = SIDE_1_COLUMN;
      }
      Call(game, event, BattleRoutine::ARMY_POSITION, column, NAME_ROW);
      auto const name{ Call(game, event, BattleRoutine::UNIT_NAME, locals.At(0), unit) };
      Call(game, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_ready_name_format), SideColorCode(side), name);
      Call(game, event, BattleRoutine::ARMY_POSITION, column, READY_ROW);
      Call(game, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_ready_text));
      announced = true;
    }
  if (announced) {
    Call(game, event, BattleRoutine::BATTLE_PLAYER_TURN);
    Call(game, event, BattleRoutine::DELAY_UNITS, PAUSE);
  }
  return announced;
}

uint16_t svc_btlready(Game& game, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(game, address, args, count);
}

auto port_routine_opeu_battle_announce_ready(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_battle_announce_ready(game, svc_btlready);
}
