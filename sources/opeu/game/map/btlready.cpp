// The round's "ready for battle" announcements.
//
// Every engaged unit on a player's side whose level gap equals the round is announced ("<unit> is ready for
// battle!"), then the game pauses. Returns 1 when anything was announced.
#include "map/btlready.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "io/waitkey.hpp"
#include "map/btlcmdr.hpp"
#include "map/btllist.hpp"
#include "map/btlplay.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"
#include "units/unitname.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>
#include <ranges>

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

uint16_t opeu_battle_announce_ready(Game& game) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ NAME_BYTES };
  bool                announced{ false };
  for (auto const list : std::views::iota(uint8_t{ 0 }, SIDE_COUNT))
    for (auto const slot : std::views::iota(uint8_t{ 0 }, BATTLE_LIST_LENGTH)) {
      auto const unit{ BattleList(data, list, slot) };
      if (!unit) continue;
      auto const side{ opeu_battle_unit_side(game, unit) };
      if (data.side_status[side] != SIDE_PLAYED) continue;
      if (opeu_battle_level_gap(game, Record<Unit>(data, unit).first) != data.battle_level_limit) continue;
      uint16_t column;
      if (!side) {
        opeu_battle_name_clear_left(game);
        column = SIDE_0_COLUMN;
      } else {
        opeu_battle_name_clear_right(game);
        column = SIDE_1_COLUMN;
      }
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(column), static_cast<short>(NAME_ROW));
      auto const hoisted0{ static_cast<std::uint16_t>(locals.At(0)) };
      auto const hoisted1{ static_cast<std::uint16_t>(unit) };
      auto const name{ opeu_unit_name(game, hoisted1, hoisted0) };
      opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(Near(data, data.battle_ready_name_format)), static_cast<std::uint16_t>(SideColorCode(side)),
                                                            static_cast<std::uint16_t>(name) });
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(column), static_cast<short>(READY_ROW));
      opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(Near(data, data.battle_ready_text)) });
      announced = true;
    }
  if (announced) {
    opeu_battle_player_turn(game);
    opeu_delay_units(game, static_cast<std::uint8_t>(PAUSE));
  }
  return announced;
}
