// The row-draw callback of the element table of a regiment: name (marked when selected), strength, damage, morale,
// training and fatigue (experience).
#include "units/tblelem.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/recindex.hpp"
#include "core/tblgroup.hpp"
#include "core/textout.hpp"
#include "types.hpp"
#include "units/labels.hpp"
#include "units/table.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {
// The element name buffer of the callback's frame.
constexpr std::uint16_t NAME_TEXT{ 104 };
// Where the row names start, in columns into the window.
constexpr std::uint16_t NAME_INDENT{ 2 };
} // namespace

void opeu_tbl_draw_elements(Game& game, DrawRequest const& request) {
  auto const [column, first, last_column, last, flags]{ request };
  auto&                                 data{ Data(game) };
  TextBuffer<TABLE_SCRATCH - ROW_TEXT>  text;
  TextBuffer<TABLE_SCRATCH - NAME_TEXT> name;
  auto const&                           window{ ActiveWindow(data) };
  auto const                            layout{ TableColumnLayout(game, column, last_column) };
  if (flags & DRAW_HEADING) DrawHeading(game, window, layout, text, Near(data, data.tbl_element_header));
  for (auto const entry : DrawnRows(first, last)) {
    auto const  near{ data.row_elements[entry] };
    auto const& element{ Record<Element>(data, near) };
    auto const  side{ opeu_element_side(near) };
    if (flags & DRAW_NAMES) {
      StartRowName(game, window, entry, NAME_INDENT, side);
      if (element.flags & Bits(ElementFlag::SELECTED)) opeu_ui_text(game, Near(data, data.tbl_element_mark));
      opeu_cursor_print(game, std::array<FormatArgument, 2>{ Near(data, data.tbl_element_name), opeu_element_name(game, near, name) });
    }
    opeu_request_format(game, text, std::array<FormatArgument, 6>{ Near(data, data.tbl_element_row), element.quantity, element.damage, element.morale, element.training, element.experience });
    DrawRowText(game, window, layout, entry, text);
  }
  opeu_text_color(game, TABLE_TEXT_COLOR);
}
