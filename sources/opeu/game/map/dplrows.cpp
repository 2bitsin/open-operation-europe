// The deployment list's two far callbacks (the list window's row-draw and select): every row is selectable, and
// the rows draw the header and the elements first..last of the list: the type name (a leading mark for the selected
// element) in the side's colour, then the placed cell and the strengths, or the strengths alone for an element not
// on the field.
#include "map/dplrows.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/textout.hpp"
#include "io/listui.hpp"
#include "map/btllist.hpp"
#include "map/dplauto.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {
// DS scratch the services write: the column x, first and last column (3 bytes), then the header text.
constexpr uint16_t COLUMNS_AT{ 0xeb00 };
constexpr uint16_t TEXT_AT{ 0xeb04 };
constexpr uint16_t ROW_PIXELS{ 17 };
constexpr uint16_t HEADER_PIXELS{ 18 };
constexpr uint8_t  OWN_COLOR{ 6 };
constexpr uint8_t  PLAIN_COLOR{ 7 };
// Bits of the draw flags: the header, the names, the strengths (both of the last are drawn for the rows).
constexpr uint8_t DRAW_HEADER{ 0x03 };
constexpr uint8_t DRAW_NAMES{ 0x05 };
} // namespace

uint8_t opeu_deploy_true(Game&) {
  return 1;
}

void opeu_deploy_draw_rows(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&       data{ Data(game) };
  auto const& window{ Record<ListWindow>(data, data.list_window) };
  opeu_table_columns(game, svc_listui, static_cast<std::uint8_t>(column), static_cast<std::uint8_t>(row), COLUMNS_AT, COLUMNS_AT + 1, COLUMNS_AT + 2);
  // The strength column: the row indent, the first column and the window's left edge.
  auto const strength_x{ [&] { return static_cast<uint16_t>(Byte(game, window.widths) + Byte(game, COLUMNS_AT) + window.x - 0x20); } };
  if (flags & DRAW_HEADER) {
    opeu_request_format(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(TEXT_AT), static_cast<std::uint16_t>(Near(data, data.deploy_header)) });
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(strength_x()), static_cast<short>(window.y + 1));
    opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), PLAIN_COLOR);
    opeu_text_put_range(game, static_cast<TextHost*>(port_textout_host()), TEXT_AT, static_cast<uint8_t>(Byte(game, COLUMNS_AT + 1)), static_cast<uint8_t>(Byte(game, COLUMNS_AT + 2)));
  }
  for (unsigned index{ first }; index <= last; ++index) {
    auto const  element_at{ data.ai_element_list[index] };
    auto const& element{ Record<Element>(data, element_at) };
    auto const  y{ static_cast<uint16_t>((static_cast<int>(index) - data.list_top) * ROW_PIXELS + window.y + HEADER_PIXELS) };
    if (flags & DRAW_NAMES) {
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(window.x + 2), static_cast<short>(y));
      auto const side{ static_cast<uint8_t>(opeu_battle_element_side(game, element_at)) };
      opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), SideTextColor(side));
      if (element.flags & Bits(ElementFlag::SELECTED)) opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.deploy_mark));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.deploy_name)), static_cast<std::uint16_t>(data.element_types[element.type].name) });
    }
    // In battle the element's position word points at its placement: the cell, 0xff (or the high bit) off the field.
    auto const x{ strength_x() };
    if (auto const place{ element.position }; Byte(game, place) != BATTLE_UNPLACED && !(Byte(game, place) & BATTLE_LEFT)) {
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(x), static_cast<short>(y));
      opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), OWN_COLOR);
      auto const cell{ element.position };
      opeu_cursor_print(game, std::array<std::uint16_t, 7>{ static_cast<std::uint16_t>(Near(data, data.deploy_placed)), static_cast<std::uint16_t>(Byte(game, cell)),
                                                            static_cast<std::uint16_t>(Byte(game, static_cast<uint16_t>(cell + 1))), static_cast<std::uint16_t>(element.quantity),
                                                            static_cast<std::uint16_t>(element.morale), static_cast<std::uint16_t>(element.training), static_cast<std::uint16_t>(element.experience) });
    } else {
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<short>(x), static_cast<short>(y));
      opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), PLAIN_COLOR);
      opeu_cursor_print(game, std::array<std::uint16_t, 5>{ static_cast<std::uint16_t>(Near(data, data.deploy_waiting)), static_cast<std::uint16_t>(element.quantity),
                                                            static_cast<std::uint16_t>(element.morale), static_cast<std::uint16_t>(element.training), static_cast<std::uint16_t>(element.experience) });
    }
  }
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), PLAIN_COLOR);
}

auto port_routine_opeu_deploy_draw_rows(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  auto const byte{ [&](unsigned i) { return static_cast<uint8_t>(Arg(args, count, i)); } };
  opeu_deploy_draw_rows(game, byte(0), byte(1), byte(2), byte(3), byte(4));
  return 0;
}

auto port_routine_opeu_deploy_true(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_deploy_true(game);
}
