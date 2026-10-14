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
#include "port.hpp"
#include "services.hpp"
#include <algorithm>
#include <array>
#include <ranges>

using namespace opeu::game;

namespace {
// DS scratch: the column layout (3 bytes, still stored where the original kept it: the battle path costs share the
// bytes), then the header text the format service writes.
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

void opeu_deploy_draw_rows(Game& game, DrawRequest const& request) {
  auto const [column, first, last_column, last, flags]{ request };
  auto&       data{ Data(game) };
  auto const& window{ Record<ListWindow>(data, data.list_window) };
  auto const  layout{ TableColumnLayout(game, column, last_column) };
  std::ranges::copy(std::array{ layout.lead, layout.start, layout.count }, game.State().Bytes(COLUMNS_AT, 3).begin());
  // The strength column: the row indent, the first column and the window's left edge.
  auto const strength_x{ [&] { return static_cast<uint16_t>(Byte(game, window.widths) + layout.lead + window.x - 0x20); } };
  if (flags & DRAW_HEADER) {
    opeu_request_format(game, TEXT_AT, std::array<FormatArgument, 1>{ Near(data, data.deploy_header) });
    opeu_army_position(game, static_cast<short>(strength_x()), static_cast<short>(window.y + 1));
    opeu_text_color(game, PLAIN_COLOR);
    opeu_text_put_range(game, TEXT_AT, layout.start, layout.count);
  }
  for (auto const index : std::views::iota(unsigned{ first }, std::max(unsigned{ first }, last + 1u))) {
    auto const  element_at{ data.ai_element_list[index] };
    auto const& element{ Record<Element>(data, element_at) };
    auto const  y{ static_cast<uint16_t>((static_cast<int>(index) - data.list_top) * ROW_PIXELS + window.y + HEADER_PIXELS) };
    if (flags & DRAW_NAMES) {
      opeu_army_position(game, static_cast<short>(window.x + 2), static_cast<short>(y));
      auto const side{ static_cast<uint8_t>(opeu_battle_element_side(game, element_at)) };
      opeu_text_color(game, SideTextColor(side));
      if (element.flags & Bits(ElementFlag::SELECTED)) opeu_ui_text(game, Near(data, data.deploy_mark));
      opeu_cursor_print(game, std::array<FormatArgument, 2>{ Near(data, data.deploy_name), data.element_types[element.type].name });
    }
    // In battle the element's position word points at its placement: the cell, 0xff (or the high bit) off the field.
    auto const x{ strength_x() };
    if (auto const place{ element.position }; Byte(game, place) != BATTLE_UNPLACED && !(Byte(game, place) & BATTLE_LEFT)) {
      opeu_army_position(game, static_cast<short>(x), static_cast<short>(y));
      opeu_text_color(game, OWN_COLOR);
      auto const cell{ element.position };
      opeu_cursor_print(game, std::array<FormatArgument, 7>{ Near(data, data.deploy_placed), Byte(game, cell), Byte(game, static_cast<uint16_t>(cell + 1)), element.quantity, element.morale,
                                                             element.training, element.experience });
    } else {
      opeu_army_position(game, static_cast<short>(x), static_cast<short>(y));
      opeu_text_color(game, PLAIN_COLOR);
      opeu_cursor_print(game, std::array<FormatArgument, 5>{ Near(data, data.deploy_waiting), element.quantity, element.morale, element.training, element.experience });
    }
  }
  opeu_text_color(game, PLAIN_COLOR);
}
