// The player's deployment of one battle unit. The deployment list (its rows drawn by map/dplrows) picks an element,
// which is lifted, shown its zone and placed by hand; closing the list offers automatic placement of the elements
// still off the field and then asks whether deployment is finished.
#include "map/dplunit.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "io/keyboard.hpp"
#include "io/listdlg.hpp"
#include "map/btlcmdr.hpp"
#include "map/btllist.hpp"
#include "map/dplauto.hpp"
#include "map/dplcell.hpp"
#include "types.hpp"
#include "units/unitname.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {
// DS scratch for the unit's name and the question.
constexpr uint16_t NAME_AT{ 0xeb20 };
constexpr uint16_t QUESTION_AT{ 0xeb40 };
// The list's first row is the header; the list opens on the first row and is redrawn whole only the first time.
constexpr uint8_t FIRST_DRAW{ 7 };
constexpr uint8_t REDRAW{ 6 };

// The x byte of the element's placement (its position word points at it).
[[nodiscard]] auto PlacedX(Game& game, Element const& element) -> uint8_t {
  return Byte(game, element.position);
}
} // namespace

void opeu_deploy_unit(Game& game, uint16_t unit) {
  auto&      data{ Data(game) };
  auto const panel{ [&] { opeu_battle_commander_panel(game, static_cast<uint8_t>(data.battle_active_side), 0); } };
  auto const close_question{ [&] { opeu_text_paper_set(game, static_cast<uint8_t>(0)); } };
  panel();
  opeu_ui_text(game, Near(data, data.deploy_title));
  if (unit != Near(data, data.garrison_unit)) {
    auto const name{ opeu_unit_name(game, unit, NAME_AT) };
    auto const side{ static_cast<uint8_t>(opeu_battle_unit_side(game, unit)) };
    opeu_cursor_print(
        game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(Near(data, data.deploy_unit_title)), static_cast<std::uint16_t>(SideColorCode(side)), static_cast<std::uint16_t>(name) });
  } else {
    opeu_ui_text(game, Near(data, data.deploy_garrison_title));
  }
  data.deploy_window.rows = static_cast<uint8_t>(static_cast<std::uint16_t>(opeu_battle_element_count(game, unit)) + 1);
  {
    uint8_t row{ 0 };
    for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it, ++row) data.ai_element_list[row] = it.Near();
  }
  for (;;) {
    data.deploy_row = 0;
    auto draw{ FIRST_DRAW };
    for (;;) {
      data.list_row = data.deploy_row;
      if (opeu_list_dialog(game, data.deploy_window, { 1, static_cast<std::uint8_t>(draw) })) {
        // An element row: lift the element, show its zone and place it by hand.
        data.deploy_row = data.list_row;
        auto element{ Record<Unit>(data, unit).first };
        for ([[maybe_unused]] auto const row : std::views::iota(uint8_t{ 0 }, data.list_row)) element = Record<Element>(data, element).next;
        if (PlacedX(game, Record<Element>(data, element)) != BATTLE_UNPLACED) opeu_deploy_lift(game, element);
        opeu_deploy_show_zone(game, element);
        opeu_deploy_manual(game, element);
        opeu_deploy_hide_zone(game, element);
        draw = REDRAW;
        continue;
      }
      uint8_t all{ 0 };
      uint8_t off_field{ 0 };
      for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
        if (PlacedX(game, element) == BATTLE_UNPLACED) ++off_field;
        ++all;
      }
      if (off_field) {
        panel();
        opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(QUESTION_AT), static_cast<std::uint16_t>(Near(data, data.deploy_ask_auto)),
                                                                static_cast<std::uint16_t>(off_field == all ? Near(data, data.deploy_all) : Near(data, data.deploy_others)) });
        if (!static_cast<std::uint16_t>(opeu_prompt_yes_no(game, QUESTION_AT))) {
          close_question();
          break; // back to the list's first row
        }
        close_question();
        opeu_deploy_auto(game, unit);
      }
      panel();
      auto const done{ static_cast<std::uint16_t>(opeu_prompt_yes_no(game, Near(data, data.deploy_ask_done))) };
      close_question();
      opeu_battle_panel_side1(game);
      opeu_battle_panel_side0(game);
      if (done) return;
      draw = REDRAW;
    }
  }
}
