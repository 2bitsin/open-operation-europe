// The deployment list's two far callbacks (the list window's row-draw and select): every row is selectable, and
// the rows draw the header and the elements first..last of the list: the type name (a leading mark for the selected
// element) in the side's colour, then the placed cell and the strengths, or the strengths alone for an element not
// on the field.
#include "types.hpp"
#include "map/dplrows.hpp"
#include "common/battle-routine.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "map/dplauto.hpp"
#include "types/events.hpp"

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {
// DS scratch the services write: the column x, first and last column (3 bytes), then the header text.
constexpr uint16_t COLUMNS_AT{ 0xeb00 };
constexpr uint16_t TEXT_AT{ 0xeb04 };
constexpr uint16_t ROW_PIXELS{ 17 };
constexpr uint16_t HEADER_PIXELS{ 18 };
constexpr uint8_t  OWN_COLOR{ 6 };
constexpr uint8_t  ENEMY_COLOR{ 5 };
constexpr uint8_t  PLAIN_COLOR{ 7 };
constexpr uint8_t  OFF_FIELD{ 0xff };
constexpr uint8_t  OFF_FIELD_BIT{ 0x80 };
// Bits of the draw flags: the header, the names, the strengths (both of the last are drawn for the rows).
constexpr uint8_t DRAW_HEADER{ 0x03 };
constexpr uint8_t DRAW_NAMES{ 0x05 };
} // namespace

uint8_t opeu_deploy_true(uint8_t*, BattleEvent) {
  return 1;
}

void opeu_deploy_draw_rows(uint8_t* ds, BattleEvent event, uint8_t column,
                           uint8_t first, uint8_t row, uint8_t last,
                           uint8_t flags) {
  auto&       data{ Data(ds) };
  auto const& window{ Record<ListWindow>(data, data.list_window) };
  Call(ds, event, BattleRoutine::TABLE_COLUMNS, column, COLUMNS_AT, row,
       COLUMNS_AT + 1, COLUMNS_AT + 2);
  // The strength column: the row indent, the first column and the window's left edge.
  auto const strength_x{ [&] {
    return static_cast<uint16_t>(Byte(ds, window.widths) +
                                 Byte(ds, COLUMNS_AT) + window.x - 0x20);
  } };
  if (flags & DRAW_HEADER) {
    Call(ds, event, BattleRoutine::REQUEST_FORMAT, TEXT_AT,
         Near(data, data.deploy_header));
    Call(ds, event, BattleRoutine::ARMY_POSITION, strength_x(), window.y + 1);
    Call(ds, event, BattleRoutine::TEXT_COLOR, PLAIN_COLOR);
    Call(ds, event, BattleRoutine::TEXT_PUT_RANGE, TEXT_AT,
         Byte(ds, COLUMNS_AT + 1), Byte(ds, COLUMNS_AT + 2));
  }
  for (unsigned index{ first }; index <= last; ++index) {
    auto const  element_at{ data.ai_element_list[index] };
    auto const& element{ Record<Element>(data, element_at) };
    auto const  y{ static_cast<uint16_t>(
        (static_cast<int>(index) - data.list_top) * ROW_PIXELS + window.y +
        HEADER_PIXELS) };
    if (flags & DRAW_NAMES) {
      Call(ds, event, BattleRoutine::ARMY_POSITION, window.x + 2, y);
      auto const side{ static_cast<uint8_t>(
          Call(ds, event, BattleRoutine::BATTLE_ELEMENT_SIDE, element_at)) };
      Call(ds, event, BattleRoutine::TEXT_COLOR,
           side == 0 ? OWN_COLOR : ENEMY_COLOR);
      if (element.flags & Bits(ElementFlag::SELECTED))
        Call(ds, event, BattleRoutine::UI_TEXT, Near(data, data.deploy_mark));
      Call(ds, event, BattleRoutine::CURSOR_PRINT, Near(data, data.deploy_name),
           data.element_types[element.type].name);
    }
    // In battle the element's position word points at its placement: the cell, 0xff (or the high bit) off the field.
    auto const x{ strength_x() };
    if (auto const place{ element.position };
        Byte(ds, place) != OFF_FIELD && !(Byte(ds, place) & OFF_FIELD_BIT)) {
      Call(ds, event, BattleRoutine::ARMY_POSITION, x, y);
      Call(ds, event, BattleRoutine::TEXT_COLOR, OWN_COLOR);
      auto const cell{ element.position };
      Call(ds, event, BattleRoutine::CURSOR_PRINT,
           Near(data, data.deploy_placed), Byte(ds, cell),
           Byte(ds, static_cast<uint16_t>(cell + 1)), element.quantity,
           element.morale, element.training, element.experience);
    } else {
      Call(ds, event, BattleRoutine::ARMY_POSITION, x, y);
      Call(ds, event, BattleRoutine::TEXT_COLOR, PLAIN_COLOR);
      Call(ds, event, BattleRoutine::CURSOR_PRINT,
           Near(data, data.deploy_waiting), element.quantity, element.morale,
           element.training, element.experience);
    }
  }
  Call(ds, event, BattleRoutine::TEXT_COLOR, PLAIN_COLOR);
}

// Services of this module: the deployment group's (map/dplauto).
uint16_t svc_dplrows(uint8_t* ds, uint32_t service, const uint16_t* args,
                     unsigned int count) {
  return svc_battle_deploy(ds, service, args, count);
}

auto port_routine_opeu_deploy_draw_rows(std::uint8_t*        ds,
                                        std::uint16_t const* args,
                                        unsigned count) -> std::uint32_t {
  auto const byte{ [&](unsigned i) {
    return static_cast<uint8_t>(Arg(args, count, i));
  } };
  opeu_deploy_draw_rows(ds, svc_dplrows, byte(0), byte(1), byte(2), byte(3),
                        byte(4));
  return 0;
}

auto port_routine_opeu_deploy_true(std::uint8_t* ds, std::uint16_t const*,
                                   unsigned) -> std::uint32_t {
  return opeu_deploy_true(ds, svc_dplrows);
}
