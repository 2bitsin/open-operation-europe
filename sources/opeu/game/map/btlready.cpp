// The round's "ready for battle" announcements.
//
// Every engaged unit on a player's side whose level gap equals the round is announced ("<unit> is ready for
// battle!"), then the game pauses. Returns 1 when anything was announced.
#include "types.hpp"
#include "map/btlready.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "common/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// Below the frame: the unit name buffer, and the one-byte copy of " " the original initializes and never reads.
constexpr uint16_t NAME_AT{ 0x2c };
constexpr uint16_t BLANK_AT{ 0x2e };
// The text columns of the two sides' name panels and the rows of the two lines.
constexpr uint16_t SIDE_0_COLUMN{ 1 };
constexpr uint16_t SIDE_1_COLUMN{ 0x38 };
constexpr uint16_t NAME_ROW{ 0x156 };
constexpr uint16_t READY_ROW{ 0x168 };
constexpr uint16_t SIDE_0_COLOR{ '6' };
constexpr uint16_t SIDE_1_COLOR{ '5' };
constexpr uint16_t PAUSE{ 3 };
} // namespace

// frame: the routine's frame on the DS stack; the unit name buffer lives there.
uint16_t opeu_battle_announce_ready(uint8_t* ds, BattleEvent event,
                                    uint16_t frame) {
  auto& data{ Data(ds) };
  Byte(ds, Local(frame, BLANK_AT)) = data.battle_ready_blank;
  bool announced{ false };
  for (uint8_t list{ 0 }; list < SIDE_COUNT; ++list)
    for (uint8_t slot{ 0 }; slot < BATTLE_LIST_LENGTH; ++slot) {
      auto const unit{ BattleList(data, list, slot) };
      if (!unit)
        continue;
      auto const side{ opeu_battle_unit_side(ds, event, unit) };
      if (data.side_status[side] != SIDE_PLAYED)
        continue;
      if (opeu_battle_level_gap(ds, event, Record<Unit>(data, unit).first) !=
          data.battle_level_limit)
        continue;
      uint16_t column;
      if (!side) {
        Call(ds, event, BattleRoutine::BATTLE_NAME_CLEAR_LEFT);
        column = SIDE_0_COLUMN;
      } else {
        Call(ds, event, BattleRoutine::BATTLE_NAME_CLEAR_RIGHT);
        column = SIDE_1_COLUMN;
      }
      Call(ds, event, BattleRoutine::ARMY_POSITION, column, NAME_ROW);
      auto const name{ Call(ds, event, BattleRoutine::UNIT_NAME,
                            Local(frame, NAME_AT), unit) };
      Call(ds, event, BattleRoutine::CURSOR_PRINT,
           Near(data, data.battle_ready_name_format),
           side ? SIDE_1_COLOR : SIDE_0_COLOR, name);
      Call(ds, event, BattleRoutine::ARMY_POSITION, column, READY_ROW);
      Call(ds, event, BattleRoutine::CURSOR_PRINT,
           Near(data, data.battle_ready_text));
      announced = true;
    }
  if (announced) {
    Call(ds, event, BattleRoutine::BATTLE_PLAYER_TURN);
    Call(ds, event, BattleRoutine::DELAY_UNITS, PAUSE);
  }
  return announced;
}

uint16_t svc_btlready(uint8_t* ds, uint32_t address, const uint16_t* args,
                      unsigned int count) {
  return port_service(ds, address, args, count);
}

auto port_routine_opeu_battle_announce_ready(std::uint8_t* ds,
                                             std::uint16_t const*, unsigned)
    -> std::uint32_t {
  return WithBattleStack([&](uint16_t sp) {
    return opeu_battle_announce_ready(ds, svc_btlready, BattleFrame(sp));
  });
}
