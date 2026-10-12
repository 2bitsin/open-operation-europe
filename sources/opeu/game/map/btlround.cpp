// The end of a battle.
//
// opeu_battle_report_outcome reports the outcome of the computer-turn test (1: side 0's lead unit decides; at a
// defended city holding equipment the battle is a stalemate, otherwise the city changes hands; 2: side 1 wins),
// opeu_battle_report_stalemate reports a stalemate after the tenth round and opeu_battle_closing_music picks the
// closing music (a routed player side, a player's win or loss).
#include "map/btlround.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "map/btllist.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// COMPUTER_TURN's verdict.
constexpr uint8_t SIDE_0_DECIDES{ 1 };
constexpr uint8_t SIDE_1_WINS{ 2 };
constexpr uint8_t NO_WINNER{ 0xff };

constexpr uint16_t MUSIC_ROUTED{ 0x10 };
constexpr uint16_t MUSIC_WON{ 9 };
constexpr uint16_t MUSIC_LOST{ 10 };

// The result banner: its frame (x, y, color) and the pauses.
constexpr uint16_t BANNER_Y{ 0xb0 };
constexpr uint16_t STALEMATE_X{ 0x22 };
constexpr uint16_t STALEMATE_COLOR{ 0xb };
constexpr uint16_t VICTORY_X{ 0x21 };
constexpr uint16_t SIDE_0_COLOR{ 0xf };
constexpr uint16_t SIDE_1_COLOR{ 0x11 };
constexpr uint16_t STALEMATE_PAUSE{ 0x1e };
constexpr uint16_t VICTORY_PAUSE{ 0x28 };

// The word index `i` of a word table, read where the original reads it for any index.
[[nodiscard]] auto WordOf(uint8_t const* ds, uint16_t table, unsigned i) -> uint16_t {
  return Word(ds, static_cast<uint16_t>(table + i * 2));
}

[[nodiscard]] auto AtCity(DataSegment const& data) -> bool {
  return (MapCellAt(data, data.battle_x, data.battle_y) & Bits(MapCell::TERRAIN_MASK)) == std::to_underlying(Terrain::CITY);
}

void Stalemate(uint8_t* ds, BattleEvent event, uint16_t text) {
  Call(ds, event, BattleRoutine::BATTLE_DAILY_CONDITION);
  Call(ds, event, BattleRoutine::BATTLE_CLEAR_POSITIONS);
  Call(ds, event, BattleRoutine::CONTEXT_FRAME, STALEMATE_X, BANNER_Y, STALEMATE_COLOR);
  Call(ds, event, BattleRoutine::UI_TEXT, text);
  Call(ds, event, BattleRoutine::DELAY_UNITS, STALEMATE_PAUSE);
  Call(ds, event, BattleRoutine::MUSIC_STOP);
}
} // namespace

// side: the winning list (0 or 1).
void opeu_battle_closing_music(uint8_t* ds, BattleEvent event, uint8_t side) {
  auto& data{ Data(ds) };
  Call(ds, event, BattleRoutine::MUSIC_STOP);
  uint16_t track;
  if ((data.side_status[0] == SIDE_PLAYED && data.battle_side_routed[0]) || (data.side_status[1] == SIDE_PLAYED && data.battle_side_routed[1])) {
    track = MUSIC_ROUTED;
  } else {
    auto const lead{ WordOf(ds, Near(data, data.battle_lists), side * BATTLE_LIST_LENGTH) };
    auto const winner{ opeu_battle_unit_side(ds, event, lead) };
    track = WordOf(ds, Near(data, data.side_status), winner) == SIDE_PLAYED ? MUSIC_WON : MUSIC_LOST;
  }
  Call(ds, event, BattleRoutine::SET_TURN_CONTEXT, track);
  Call(ds, event, BattleRoutine::FINISH_SIDE_INITIALIZATION, 0);
}

void opeu_battle_report_outcome(uint8_t* ds, BattleEvent event) {
  auto&      data{ Data(ds) };
  auto const result{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_COMPUTER_TURN)) };
  if (result == SIDE_0_DECIDES) {
    data.battle_side = opeu_battle_unit_side(ds, event, data.battle_lists[0]);
    if (AtCity(data)) {
      auto const near{ static_cast<uint16_t>(Call(ds, event, BattleRoutine::FIND_OBJECTIVE_AT, data.battle_x, data.battle_y)) };
      auto&      objective{ Record<Objective>(data, near) };
      for (auto const& item : objective.items)
        if (item.quantity) {
          data.battle_side = NO_WINNER;
          Stalemate(ds, event, Near(data, data.battle_siege_stalemate_text));
          return;
        }
      objective.flags ^= Bits(ObjectiveFlag::SIDE_1);
      Call(ds, event, BattleRoutine::SIDE_GROUPS_RETARGET, 0);
      Call(ds, event, BattleRoutine::SIDE_GROUPS_RETARGET, 1);
    }
  }
  if (result == SIDE_1_WINS) {
    auto const unit{ BattleList(data, 1, 0) ? BattleList(data, 1, 0) : BattleList(data, 1, 1) };
    data.battle_side = opeu_battle_unit_side(ds, event, unit);
    if (AtCity(data)) Call(ds, event, BattleRoutine::BATTLE_RETURN_GARRISON);
  }
  Call(ds, event, BattleRoutine::BATTLE_DAILY_CONDITION);
  Call(ds, event, BattleRoutine::BATTLE_CLEAR_POSITIONS);
  Call(ds, event, BattleRoutine::CONTEXT_FRAME, VICTORY_X, BANNER_Y, data.battle_side ? SIDE_1_COLOR : SIDE_0_COLOR);
  Call(ds, event, BattleRoutine::CURSOR_PRINT, Near(data, data.battle_victory_format), WordOf(ds, Near(data, data.side_names), data.battle_side));
  opeu_battle_closing_music(ds, event, static_cast<uint8_t>(result - 1));
  Call(ds, event, BattleRoutine::DELAY_UNITS, VICTORY_PAUSE);
}

void opeu_battle_report_stalemate(uint8_t* ds, BattleEvent event) {
  auto& data{ Data(ds) };
  data.battle_side = NO_WINNER;
  if (AtCity(data)) Call(ds, event, BattleRoutine::BATTLE_RETURN_GARRISON);
  Stalemate(ds, event, Near(data, data.battle_stalemate_text));
}

uint16_t svc_btlround(uint8_t* ds, uint32_t address, const uint16_t* args, unsigned int count) {
  return port_service(ds, address, args, count);
}
