// The end of a battle.
//
// opeu_battle_report_outcome reports the outcome of the computer-turn test (1: side 0's lead unit decides; at a
// defended city holding equipment the battle is a stalemate, otherwise the city changes hands; 2: side 1 wins),
// opeu_battle_report_stalemate reports a stalemate after the tenth round and opeu_battle_closing_music picks the
// closing music (a routed player side, a player's win or loss).
#include "map/btlround.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/objrank.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "map/btllist.hpp"
#include "map/btlturn.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

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
[[nodiscard]] auto WordOf(Game const& game, uint16_t table, unsigned i) -> uint16_t {
  return Word(game, static_cast<uint16_t>(table + i * 2));
}

void Stalemate(Game& game, uint16_t text) {
  opeu_battle_daily_condition(game);
  opeu_battle_clear_positions(game);
  opeu_context_frame(game, static_cast<const UiHost*>(port_uiprim_host()), STALEMATE_X, BANNER_Y, STALEMATE_COLOR);
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), text);
  opeu_delay_units(game, static_cast<std::uint8_t>(STALEMATE_PAUSE));
  opeu_music_stop(game);
}
} // namespace

// side: the winning list (0 or 1).
void opeu_battle_closing_music(Game& game, uint8_t side) {
  auto& data{ Data(game) };
  opeu_music_stop(game);
  uint16_t track;
  if ((data.side_status[0] == SIDE_PLAYED && data.battle_side_routed[0]) || (data.side_status[1] == SIDE_PLAYED && data.battle_side_routed[1])) {
    track = MUSIC_ROUTED;
  } else {
    auto const lead{ WordOf(game, Near(data, data.battle_lists), side * BATTLE_LIST_LENGTH) };
    auto const winner{ opeu_battle_unit_side(game, lead) };
    track = WordOf(game, Near(data, data.side_status), winner) == SIDE_PLAYED ? MUSIC_WON : MUSIC_LOST;
  }
  opeu_set_turn_context(game, static_cast<std::uint8_t>(track));
  opeu_finish_side_initialization(game, static_cast<std::uint8_t>(0));
}

void opeu_battle_report_outcome(Game& game) {
  auto&      data{ Data(game) };
  auto const result{ static_cast<uint8_t>(opeu_battle_computer_turn(game)) };
  if (result == SIDE_0_DECIDES) {
    data.battle_side = opeu_battle_unit_side(game, data.battle_lists[0]);
    if (BattleOnCity(data)) {
      auto const near{ static_cast<uint16_t>(opeu_find_objective_at(game, static_cast<uint8_t>(data.battle_x), static_cast<uint8_t>(data.battle_y))) };
      auto&      objective{ Record<Objective>(data, near) };
      for (auto const& item : objective.items)
        if (item.quantity) {
          data.battle_side = NO_WINNER;
          Stalemate(game, Near(data, data.battle_siege_stalemate_text));
          return;
        }
      objective.flags ^= Bits(ObjectiveFlag::SIDE_1);
      opeu_side_groups_retarget(game, static_cast<uint8_t>(0));
      opeu_side_groups_retarget(game, static_cast<uint8_t>(1));
    }
  }
  if (result == SIDE_1_WINS) {
    auto const unit{ BattleList(data, 1, 0) ? BattleList(data, 1, 0) : BattleList(data, 1, 1) };
    data.battle_side = opeu_battle_unit_side(game, unit);
    if (BattleOnCity(data)) opeu_battle_return_garrison(game);
  }
  opeu_battle_daily_condition(game);
  opeu_battle_clear_positions(game);
  opeu_context_frame(game, static_cast<const UiHost*>(port_uiprim_host()), VICTORY_X, BANNER_Y, data.battle_side ? SIDE_1_COLOR : SIDE_0_COLOR);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.battle_victory_format)),
                                                        static_cast<std::uint16_t>(WordOf(game, Near(data, data.side_names), data.battle_side)) });
  opeu_battle_closing_music(game, static_cast<uint8_t>(result - 1));
  opeu_delay_units(game, static_cast<std::uint8_t>(VICTORY_PAUSE));
}

void opeu_battle_report_stalemate(Game& game) {
  auto& data{ Data(game) };
  data.battle_side = NO_WINNER;
  if (BattleOnCity(data)) opeu_battle_return_garrison(game);
  Stalemate(game, Near(data, data.battle_stalemate_text));
}
