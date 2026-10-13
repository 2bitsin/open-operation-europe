// The battle cursor's status line: the cell, its terrain and garrison share, and the element on it when the active
// side may see it.
#include "map/btlinfo.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "map/btlcalc.hpp"
#include "map/btlcmdr.hpp"
#include "map/btllist.hpp"
#include "map/btlunit.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "map/btlcmds.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {
// The cell terrain class of the objective's cells.
constexpr uint8_t  OBJECTIVE_TERRAIN{ 10 };
constexpr uint16_t PAPER{ 4 };
constexpr uint16_t PAPER_OFF{ 0 };
constexpr uint16_t LABEL_COLOR{ 7 };
constexpr uint16_t LINE_X{ 2 };
constexpr uint16_t CELL_LINE_Y{ 0x50 };
constexpr uint16_t TYPE_LINE_Y{ 0x68 };
constexpr uint16_t QUANTITY_LINE_Y{ 0x78 };
} // namespace

void opeu_battle_cursor_info(Game& game) {
  auto&      data{ Data(game) };
  auto const x{ data.battle_cursor_x };
  auto const y{ data.battle_cursor_y };
  auto const terrain{ static_cast<uint8_t>(opeu_battle_cell_terrain(game, svc_btlcalc, static_cast<uint8_t>(x), static_cast<uint8_t>(y))) };
  opeu_battle_info_clear(game);
  opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(PAPER));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(LINE_X), static_cast<short>(CELL_LINE_Y));
  opeu_cursor_print(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(Near(data, data.battle_fmt_cursor)), static_cast<std::uint16_t>(data.battle_cursor_x),
                                                        static_cast<std::uint16_t>(data.battle_cursor_y), static_cast<std::uint16_t>(data.battle_terrain_names[terrain]) });
  if (terrain == OBJECTIVE_TERRAIN)
    for (unsigned i{ 0 }; i < BATTLE_EXTRA_COUNT; ++i) {
      auto const& extra{ BattleExtraAt(data, i) };
      if (extra.x == BATTLE_UNPLACED) break;
      if (extra.x == data.battle_cursor_x && extra.y == data.battle_cursor_y) {
        opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.battle_fmt_extra)), static_cast<std::uint16_t>(extra.count) });
        break;
      }
    }
  auto const element{ opeu_battle_element_at(game, svc_btlcalc, static_cast<uint8_t>(data.battle_cursor_x), static_cast<uint8_t>(data.battle_cursor_y)) };
  if (element && (opeu_battle_element_side(game, element) == data.battle_active_side || opeu_battle_element_flag2(game, element))) {
    opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), SideTextColor(opeu_battle_element_side(game, element)));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(LINE_X), static_cast<short>(TYPE_LINE_Y));
    auto const& record{ Record<Element>(data, element) };
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), data.element_types[record.type].name);
    opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), LABEL_COLOR);
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(LINE_X), static_cast<short>(QUANTITY_LINE_Y));
    opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.battle_fmt_cell_ok)), static_cast<std::uint16_t>(record.quantity) });
  }
  opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(PAPER_OFF));
}
