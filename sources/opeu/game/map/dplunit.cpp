// The player's deployment of one battle unit. The deployment list (its rows drawn by map/dplrows) picks an element,
// which is lifted, shown its zone and placed by hand; closing the list offers automatic placement of the elements
// still off the field and then asks whether deployment is finished.
#include "map/dplunit.hpp"
#include "common/battle-field.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "map/dplauto.hpp"
#include "map/dplcell.hpp"
#include "types.hpp"

#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// DS scratch for the unit's name and the question.
constexpr uint16_t NAME_AT{ 0xeb20 };
constexpr uint16_t QUESTION_AT{ 0xeb40 };
// The list's first row is the header; the list opens on the first row and is redrawn whole only the first time.
constexpr uint8_t FIRST_DRAW{ 7 };
constexpr uint8_t REDRAW{ 6 };

// The x byte of the element's placement (its position word points at it).
[[nodiscard]] auto PlacedX(uint8_t* ds, Element const& element) -> uint8_t {
  return Byte(ds, element.position);
}
} // namespace

void opeu_deploy_unit(uint8_t* ds, BattleEvent event, uint16_t unit) {
  auto&      data{ Data(ds) };
  auto const panel{ [&] { Call(ds, event, BattleRoutine::BATTLE_COMMANDER_PANEL, data.battle_active_side, 0); } };
  auto const close_question{ [&] { Call(ds, event, BattleRoutine::TEXT_PAPER_SET, 0); } };
  panel();
  Call(ds, event, BattleRoutine::UI_TEXT, Near(data, data.deploy_title));
  if (unit != Near(data, data.garrison_unit)) {
    auto const name{ Call(ds, event, BattleRoutine::UNIT_NAME, unit, NAME_AT) };
    auto const side{ static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_UNIT_SIDE, unit)) };
    Call(ds, event, BattleRoutine::CURSOR_PRINT, Near(data, data.deploy_unit_title), SideColorCode(side), name);
  } else {
    Call(ds, event, BattleRoutine::UI_TEXT, Near(data, data.deploy_garrison_title));
  }
  data.deploy_window.rows = static_cast<uint8_t>(Call(ds, event, BattleRoutine::BATTLE_ELEMENT_COUNT, unit) + 1);
  {
    uint8_t row{ 0 };
    for (auto it{ Elements(data, Record<Unit>(data, unit)).begin() }; it != std::default_sentinel; ++it, ++row) data.ai_element_list[row] = it.Near();
  }
  for (;;) {
    data.deploy_row = 0;
    auto draw{ FIRST_DRAW };
    for (;;) {
      data.list_row = data.deploy_row;
      if (Call(ds, event, BattleRoutine::LIST_DIALOG, Near(data, data.deploy_window), 1, draw)) {
        // An element row: lift the element, show its zone and place it by hand.
        data.deploy_row = data.list_row;
        auto element{ Record<Unit>(data, unit).first };
        for (uint8_t row{ 0 }; row < data.list_row; ++row) element = Record<Element>(data, element).next;
        if (PlacedX(ds, Record<Element>(data, element)) != BATTLE_UNPLACED) opeu_deploy_lift(ds, event, element);
        opeu_deploy_show_zone(ds, event, element);
        opeu_deploy_manual(ds, event, element);
        opeu_deploy_hide_zone(ds, event, element);
        draw = REDRAW;
        continue;
      }
      uint8_t all{ 0 };
      uint8_t off_field{ 0 };
      for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
        if (PlacedX(ds, element) == BATTLE_UNPLACED) ++off_field;
        ++all;
      }
      if (off_field) {
        panel();
        Call(ds, event, BattleRoutine::REQUEST_FORMAT, QUESTION_AT, Near(data, data.deploy_ask_auto), off_field == all ? Near(data, data.deploy_all) : Near(data, data.deploy_others));
        if (!Call(ds, event, BattleRoutine::PROMPT_YES_NO, QUESTION_AT)) {
          close_question();
          break; // back to the list's first row
        }
        close_question();
        opeu_deploy_auto(ds, event, unit);
      }
      panel();
      auto const done{ Call(ds, event, BattleRoutine::PROMPT_YES_NO, Near(data, data.deploy_ask_done)) };
      close_question();
      Call(ds, event, BattleRoutine::BATTLE_PANEL_SIDE1);
      Call(ds, event, BattleRoutine::BATTLE_PANEL_SIDE0);
      if (done) return;
      draw = REDRAW;
    }
  }
}

// Services of this module: the deployment group's (map/dplauto).
uint16_t svc_dplunit(uint8_t* ds, uint32_t service, const uint16_t* args, unsigned int count) {
  return svc_battle_deploy(ds, service, args, count);
}
