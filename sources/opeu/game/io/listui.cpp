// List windows. A ListWindow has its left column (8-pixel units), top, right column and bottom, its width bytes (each
// 0x20 more than the width: the row width, then one per column, 0-terminated), the row count and far row-draw and
// select routines. list_window is the active window; list_column and list_top the first visible column and row,
// list_columns and list_rows the visible counts and list_row the selected row. list_mouse is set while the mouse
// scroll arrows show: a 16-pixel strip at the window's left edge with 32-pixel up and down arrows.
#include "io/listui.hpp"

#include "common/data-segment.hpp"
#include "common/dispatch.hpp"
#include "common/game.hpp"
#include "common/host.hpp"
#include "core/limits.hpp"
#include "core/suptable.hpp"
#include "core/tblgroup.hpp"
#include "core/tblpers.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "io/list-keys.hpp"
#include "io/mouse.hpp"
#include "io/tblsave.hpp"
#include "link/glue.hpp"
#include "map/dplrows.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/genpick.hpp"
#include "units/rqarmtbl.hpp"
#include "units/rqsuptbl.hpp"
#include "units/tblelem.hpp"
#include "units/tblstr.hpp"
#include "units/tblsupp.hpp"
#include "units/tbltype.hpp"
#include "units/tblunit.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <ranges>

using namespace opeu::game;

namespace {

// The arrow tiles (data/fg3885): far segment and offsets.
constexpr std::uint16_t ARROW_SEGMENT{ 0x3885 };
constexpr std::uint16_t ARROW_UP{ 0x00 };
constexpr std::uint16_t ARROW_UP_LIT{ 0x40 };
constexpr std::uint16_t ARROW_DOWN{ 0x80 };
constexpr std::uint16_t ARROW_DOWN_LIT{ 0xc0 };

constexpr std::uint16_t ROW_HEIGHT{ 0x11 };
constexpr std::uint8_t  WIDTH_BIAS{ 0x20 };
constexpr std::uint16_t COLOUR{ 7 };
constexpr std::uint16_t INVERT_MODE{ 0x107 };

// what the draw routine is asked to draw
enum Area : std::uint8_t {
  ALL          = 1,
  COLUMNS      = 2,
  ROWS         = 4,
  SELECTED_ROW = 8,
  SCROLL_LEFT  = 0x10, // the scroll steps, before they become COLUMNS or ROWS
  SCROLL_RIGHT = 0x20,
  SCROLL_UP    = 0x30,
  SCROLL_DOWN  = 0x40,
};

// Width byte i of a window.
auto WidthByte(Game& game, ListWindow const& window, std::uint32_t i) -> std::uint8_t& {
  return Byte(game, static_cast<std::uint16_t>(window.widths + i));
}

// The row-draw routines, by the enumerator a window's draw slot holds: each gets the column, first row, last column, last
// row and the area flags.
using RowDraw            = auto (*)(Game&, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t, std::uint8_t) -> void;
constexpr auto ROW_DRAWS = DispatchRows<ListDraw, RowDraw>({
    { ListDraw::GENERALS,      opeu_general_list_draw_rows },
    { ListDraw::GROUPS,        opeu_tbl_draw_groups        },
    { ListDraw::MISSIONS,      opeu_tbl_draw_missions      },
    { ListDraw::UNITS,         opeu_tbl_draw_units         },
    { ListDraw::ORDERS,        opeu_tbl_draw_orders        },
    { ListDraw::ELEMENTS,      opeu_tbl_draw_elements      },
    { ListDraw::STAFF,         opeu_tbl_draw_staff         },
    { ListDraw::TYPES,         opeu_tbl_draw_types         },
    { ListDraw::SUPPLIES,      opeu_tbl_draw_supplies      },
    { ListDraw::STRENGTH,      opeu_tbl_draw_strength      },
    { ListDraw::SAVES,         opeu_tbl_draw_saves         },
    { ListDraw::SUPPLY_TYPES,  opeu_supply_draw_types      },
    { ListDraw::SUPPLY_SLOTS,  opeu_supply_draw_slots      },
    { ListDraw::REQUEST_TYPES, opeu_rq_draw_types          },
    { ListDraw::REQUEST_UNITS, opeu_rq_draw_units          },
    { ListDraw::DEPLOYMENT,    opeu_deploy_draw_rows       },
});

// What a row-draw routine is asked to draw: columns column..last_column of rows row..last_row, and the area flags.
struct DrawRequest {
  std::uint8_t column;
  std::uint8_t row;
  std::uint8_t last_column;
  std::uint8_t last_row;
  std::uint8_t area;
};

auto DrawRows(Game& game, ListWindow const& window, DrawRequest const& request) -> void {
  Dispatch(ROW_DRAWS, ListDraw{ window.draw }, game, request.column, request.row, request.last_column, request.last_row, request.area);
}

// The scroll strip at a window's left edge: its pixel column and the rows its arrows span.
struct ScrollStrip {
  std::uint16_t x;
  std::uint16_t top;
  std::uint16_t bottom;
};

auto StripOf(ListWindow const& window) -> ScrollStrip {
  return { static_cast<std::uint16_t>(window.x << 3), window.y, window.bottom };
}

// Which arrow of the strip is lit.
enum class ArrowLight : std::uint8_t {
  UP,
  DOWN,
  NONE,
};

// Draws the up and the down arrow of the strip, the one `lit` names lit.
auto DrawArrows(Game& game, ScrollStrip const& strip, ArrowLight lit) -> void {
  auto&               data{ Data(game) };
  std::uint16_t const column{ static_cast<std::uint16_t>(strip.x >> 3) };
  SetStream(data, ARROW_SEGMENT, lit == ArrowLight::UP ? ARROW_UP_LIT : ARROW_UP);
  opeu_draw_far(column, strip.top, 2, 0x20, 7, port_far(data.stream_ptr[1], data.stream_ptr[0]));
  SetStream(data, ARROW_SEGMENT, lit == ArrowLight::DOWN ? ARROW_DOWN_LIT : ARROW_DOWN);
  opeu_draw_far(column, static_cast<std::uint16_t>(strip.bottom - 0x1f), 2, 0x20, 7, port_far(data.stream_ptr[1], data.stream_ptr[0]));
}

// Outlines the scroll strip and draws both arrows unlit, while the mouse scroll arrows show.
auto DrawScrollbar(Game& game, ScrollStrip const& strip) -> void {
  if (!Data(game).list_mouse) return;
  opeu_ui_outline(game, strip.x, strip.top, static_cast<std::uint16_t>(strip.x + 15), strip.bottom, COLOUR);
  DrawArrows(game, strip, ArrowLight::NONE);
}

// Redraws the arrows with the mouse pointer hidden.
auto LightArrows(Game& game, ScrollStrip const& strip, ArrowLight lit) -> void {
  if (!Data(game).list_mouse) return;
  opeu_mouse_hide(game);
  DrawArrows(game, strip, lit);
  opeu_mouse_show(game);
}

// The pixel column of the grid's left edge, right of the scroll strip.
auto GridLeft(ListWindow const& window) -> std::uint16_t {
  return static_cast<std::uint16_t>(StripOf(window).x + 0x10);
}

auto GridRight(ListWindow const& window) -> std::uint16_t {
  return static_cast<std::uint16_t>(window.right << 3);
}

// Draws a line under each row that fits, at most the window's row count; answers how many.
auto DrawRowLines(ListWindow const& window) -> std::uint8_t {
  auto const   row_y{ [&](std::uint32_t row) { return static_cast<std::uint16_t>(window.y + row * ROW_HEIGHT); } };
  auto const   fits{ [&](std::uint16_t y) { return y <= window.bottom; } };
  std::uint8_t lines{ 0 };
  std::ranges::for_each(std::views::iota(1u, window.rows + 1u) | std::views::transform(row_y) | std::views::take_while(fits), [&](std::uint16_t y) {
    opeu_ui_line(GridLeft(window), y, GridRight(window), y, COLOUR);
    lines++;
  });
  return lines;
}

// Draws the column separators from first_column while they fit: one after the row width, then one after each column up
// to the widths' terminator; answers how many.
auto DrawColumnLines(Game& game, ListWindow const& window, std::uint8_t first_column) -> std::uint8_t {
  auto const    widths{ game.State().Bytes(window.widths) };
  auto const    right{ GridRight(window) };
  auto const    line{ [&](std::uint16_t x) { opeu_ui_line(x, window.y, x, window.bottom, COLOUR); } };
  std::uint16_t x = ((widths.front() - WIDTH_BIAS) << 3) + GridLeft(window);
  if (x > right) return 0;
  line(x);
  std::uint8_t lines{ 1 };
  for (auto const width : widths.subspan(1u + first_column) | std::views::take_while([](std::uint8_t byte) { return byte != 0; })) {
    x += (width - WIDTH_BIAS) << 3;
    if (x > right) break;
    line(x);
    lines++;
  }
  return lines;
}

// Draws the window grid from first_column: the scroll strip, the outline, a line under each row that fits (setting
// list_rows) and the column separators (setting list_columns). Returns the previous visible column count.
auto DrawFrame(Game& game, ListWindow const& window, std::uint8_t first_column) -> std::uint8_t {
  auto&      data{ Data(game) };
  auto const saved{ data.list_columns };
  DrawScrollbar(game, StripOf(window));
  opeu_ui_outline(game, GridLeft(window), window.y, GridRight(window), window.bottom, COLOUR);
  data.list_rows    = static_cast<std::uint8_t>(DrawRowLines(window) - 1);
  data.list_columns = static_cast<std::uint8_t>(DrawColumnLines(game, window, first_column) - 1);
  return saved;
}
} // namespace

// The layout of table column `column` of the active list window: its offset (8-pixel units, from 2) from the first
// visible column, and the total widths of the columns 0..rows before and from `column`.
auto TableColumnLayout(Game& game, std::uint8_t column, std::uint8_t rows) -> TableColumns {
  auto&        data{ Data(game) };
  auto const&  window{ ListWindowAt(data, data.list_window) };
  auto const   width{ [&](std::uint32_t i) { return static_cast<std::uint8_t>(WidthByte(game, window, i + 1) - WIDTH_BIAS); } };
  TableColumns layout{ 2, 0, 0 };
  for (auto const i : std::views::iota(data.list_column, std::max(data.list_column, column))) layout.lead += width(i);
  for (auto const i : std::views::iota(0u, rows + 1u)) (i < column ? layout.start : layout.count) += width(i);
  return layout;
}

// The service form of TableColumnLayout: stores the three bytes at the DS offsets `first`, `second` and `third`.
void opeu_table_columns(Game& game, std::uint8_t column, std::uint8_t rows, std::uint16_t first, std::uint16_t second, std::uint16_t third) {
  auto const layout{ TableColumnLayout(game, column, rows) };
  Byte(game, first)  = layout.lead;
  Byte(game, second) = layout.start;
  Byte(game, third)  = layout.count;
}

namespace {

// Where the pointer is over a list window.
enum class Pointer : std::uint8_t {
  ELSEWHERE,
  UP_ARROW,
  DOWN_ARROW,
  ROWS,
};

auto LightOf(Pointer pointer) -> ArrowLight {
  switch (pointer) {
  case Pointer::UP_ARROW:   return ArrowLight::UP;
  case Pointer::DOWN_ARROW: return ArrowLight::DOWN;
  default:                  return ArrowLight::NONE;
  }
}

// The key a press answers over `pointer`: a left click on an arrow scrolls, on the rows chooses (elsewhere it answers the
// buttons), a right click cancels; 0 without a press.
auto PressKey(Game& game, Pointer pointer) -> std::uint8_t {
  auto const buttons{ opeu_mouse_buttons(game) };
  if (buttons & 1) {
    switch (pointer) {
    case Pointer::UP_ARROW:   return KEY_SCROLL_UP;
    case Pointer::DOWN_ARROW: return KEY_SCROLL_DOWN;
    case Pointer::ROWS:       return KEY_ENTER;
    default:                  return buttons;
    }
  }
  if (buttons & 2) return KEY_ZERO;
  return 0;
}

// Where the pointer at (x, y) is over the active window; on a row other than the selected one, the key that moves the
// selection towards it.
struct PointerPlace {
  Pointer      pointer;
  std::uint8_t move_key;
};

auto PlaceOf(Game& game, ListWindow const& window, MousePoint const& mouse) -> PointerPlace {
  auto const& data{ Data(game) };
  auto const  strip{ StripOf(window) };
  auto const [x, y]{ mouse };
  bool const on_strip{ data.list_mouse && x >= strip.x && static_cast<std::uint16_t>(strip.x + 15) >= x };
  if (on_strip && y >= strip.top && static_cast<std::uint16_t>(strip.top + 0x1f) >= y) return { Pointer::UP_ARROW, 0 };
  if (on_strip && static_cast<std::uint16_t>(strip.bottom - 0x1f) <= y && y <= strip.bottom) return { Pointer::DOWN_ARROW, 0 };
  auto const rows_bottom{ opeu_min16(static_cast<std::uint16_t>(window.rows * ROW_HEIGHT + strip.top), strip.bottom) };
  if (static_cast<std::uint16_t>(strip.x + 16) > x || static_cast<std::uint16_t>((WidthByte(game, window, 0) << 3) + strip.x - 0xf1) < x || static_cast<std::uint16_t>(strip.top + ROW_HEIGHT) > y
      || rows_bottom < y)
    return { Pointer::ELSEWHERE, 0 };
  std::uint16_t const row_y = (data.list_row - data.list_top + 1) * ROW_HEIGHT + strip.top;
  if (row_y > y) return { Pointer::ROWS, KEY_UP };
  if (static_cast<std::uint16_t>(row_y + ROW_HEIGHT) < y) return { Pointer::ROWS, KEY_DOWN };
  return { Pointer::ROWS, 0 };
}
} // namespace

// Tracks the mouse over the active window until a button press; returns KEY_SCROLL_UP or KEY_SCROLL_DOWN (left click
// on an arrow), KEY_ENTER (left click on the selected row), KEY_ZERO (right click), or KEY_UP or KEY_DOWN when the
// pointer is over a row above or below the selection. The polling loop ends on the first answer.
auto opeu_list_mouse_key(Game& game) -> std::uint8_t {
  auto&        data{ Data(game) };
  auto const&  window{ ListWindowAt(data, data.list_window) };
  auto const   strip{ StripOf(window) };
  auto         shown{ Pointer::ELSEWHERE };
  std::uint8_t key{ 0 };
  opeu_mouse_show(game);
  while (key == 0) {
    auto const place{ PlaceOf(game, window, MousePosition(game)) };
    auto const on_arrow{ place.pointer == Pointer::UP_ARROW || place.pointer == Pointer::DOWN_ARROW };
    if (on_arrow ? place.pointer != shown : shown == Pointer::UP_ARROW || shown == Pointer::DOWN_ARROW) LightArrows(game, strip, LightOf(place.pointer));
    shown = place.pointer;
    key   = place.move_key ? place.move_key : PressKey(game, shown);
  }
  opeu_mouse_hide(game);
  return key;
}

// Inverts the selected row.
void opeu_list_invert(Game& game) {
  auto&       data{ Data(game) };
  auto const& window{ ListWindowAt(data, data.list_window) };
  opeu_ui_highlight(static_cast<std::uint16_t>(window.x + 2), static_cast<std::uint16_t>((data.list_row - data.list_top + 1) * 17 + window.y),
                    static_cast<std::uint16_t>(WidthByte(game, window, 0) - WIDTH_BIAS), 0x12, INVERT_MODE);
}

namespace {

// The window's drawn area, in 8-pixel columns from x to right and pixels from y to bottom.
struct Body {
  std::uint16_t x;
  std::uint16_t y;
  std::uint16_t right;
  std::uint16_t bottom;
};

auto BodyOf(ListWindow const& window) -> Body {
  return { static_cast<std::uint16_t>(window.x + 2), window.y, window.right, window.bottom };
}

auto Height(Body const& body) -> std::uint16_t {
  return static_cast<std::uint16_t>(body.bottom - body.y);
}

// Clears the window (with the scroll strip while it shows) for a redraw.
auto ClearWindow(Game& game, ListWindow const& window) -> void {
  auto const body{ BodyOf(window) };
  if (Data(game).list_mouse)
    opeu_ui_box(static_cast<std::uint16_t>(body.x - 2), body.y, static_cast<std::uint16_t>(body.right - body.x + 2), Height(body), 0);
  else
    opeu_ui_box(body.x, body.y, static_cast<std::uint16_t>(body.right - body.x), Height(body), 0);
}

// Moves the columns from `column` on one column to the left and clears the uncovered strip at the right.
auto ShiftColumnsLeft(Game& game, ListWindow const& window, std::uint8_t column) -> void {
  auto const          body{ BodyOf(window) };
  std::uint16_t const to   = WidthByte(game, window, 0) + body.x - WIDTH_BIAS;
  std::uint16_t const from = to + WidthByte(game, window, column) - WIDTH_BIAS;
  std::uint16_t const kept = body.right - from;
  opeu_screen_copy(from, body.y, kept, Height(body), to, body.y);
  opeu_ui_box(static_cast<std::uint16_t>(kept + to), body.y, static_cast<std::uint16_t>(body.right - kept - to), Height(body), 0);
}

// Moves the columns one column to the right, making room for `column`, and clears that column.
auto ShiftColumnsRight(Game& game, ListWindow const& window, std::uint8_t column) -> void {
  auto const          body{ BodyOf(window) };
  std::uint16_t const width{ static_cast<std::uint16_t>(WidthByte(game, window, column + 1u) - WIDTH_BIAS) };
  std::uint16_t const from = WidthByte(game, window, 0) + body.x - WIDTH_BIAS;
  std::uint16_t const to   = from + width;
  std::uint16_t const kept = body.right - to;
  opeu_screen_copy(from, body.y, kept, Height(body), to, body.y);
  opeu_ui_box(from, body.y, width, Height(body), 0);
}

// Moves the rows one row up and clears the last row.
auto ShiftRowsUp(ListWindow const& window) -> void {
  auto const body{ BodyOf(window) };
  auto const width{ static_cast<std::uint16_t>(body.right - body.x) };
  opeu_screen_copy(body.x, static_cast<std::uint16_t>(body.y + 0x23), width, static_cast<std::uint16_t>(Height(body) - 0x23), body.x, static_cast<std::uint16_t>(body.y + 0x12));
  opeu_ui_box(body.x, static_cast<std::uint16_t>(body.bottom - 0x10), width, 0x10, 0);
}

// Moves the rows one row down and clears the first row.
auto ShiftRowsDown(ListWindow const& window) -> void {
  auto const body{ BodyOf(window) };
  auto const width{ static_cast<std::uint16_t>(body.right - body.x) };
  opeu_screen_copy(body.x, static_cast<std::uint16_t>(body.y + 0x12), width, static_cast<std::uint16_t>(Height(body) - 0x23), body.x, static_cast<std::uint16_t>(body.y + 0x23));
  opeu_ui_box(body.x, static_cast<std::uint16_t>(body.y + 0x12), width, 0x10, 0);
}

// Moves the window's drawn area for the step to (column, row) and answers what is left to draw; ALL after clearing it
// for a redraw from the origin or the current position.
auto Shift(Game& game, ListWindow const& window, DrawRequest& target) -> std::uint8_t {
  auto const& data{ Data(game) };
  if (target.column >= REDRAW_HERE) {
    if (target.column == REDRAW_ORIGIN) {
      target.column = target.row = 0;
    } else {
      target.column = data.list_column;
      target.row    = data.list_top;
    }
    ClearWindow(game, window);
    return ALL;
  }
  if (data.list_column < target.column) {
    ShiftColumnsLeft(game, window, target.column);
    return SCROLL_LEFT;
  }
  if (data.list_column > target.column) {
    ShiftColumnsRight(game, window, target.column);
    return SCROLL_RIGHT;
  }
  if (data.list_top < target.row) {
    ShiftRowsUp(window);
    return SCROLL_UP;
  }
  ShiftRowsDown(window);
  return SCROLL_DOWN;
}

// What the draw routine draws after a step to (column, row) set the visible counts: everything for ALL, the uncovered
// column or row for a scroll step; nothing when a step left shows fewer columns than before (`saved`).
auto Uncovered(DataSegment const& data, std::uint8_t step, std::uint8_t saved, DrawRequest request) -> std::optional<DrawRequest> {
  request.last_column = static_cast<std::uint8_t>(data.list_columns + request.column - 1);
  request.last_row    = static_cast<std::uint8_t>(data.list_rows + request.row - 1);
  request.area        = step;
  switch (step) {
  case SCROLL_LEFT:
    if (data.list_columns < saved) return std::nullopt;
    request.area   = COLUMNS;
    request.column = static_cast<std::uint8_t>(saved - data.list_columns + request.last_column);
    break;
  case SCROLL_RIGHT:
    request.area        = COLUMNS;
    request.last_column = request.column;
    break;
  case SCROLL_UP:
    request.area = ROWS;
    request.row  = request.last_row;
    break;
  case SCROLL_DOWN:
    request.area     = ROWS;
    request.last_row = request.row;
    break;
  }
  return request;
}

// Redraws the selected row and the grid, for a step that does not move.
auto RedrawSelection(Game& game, ListWindow const& window, std::uint8_t column) -> void {
  auto const& data{ Data(game) };
  DrawRows(game, window, { column, data.list_row, static_cast<std::uint8_t>(data.list_columns + column - 1), data.list_row, SELECTED_ROW });
  DrawFrame(game, window, column);
}
} // namespace

// Shows the window from column and row. REDRAW_ORIGIN redraws from the origin, REDRAW_HERE at the current position;
// otherwise a one-column or one-row step scrolls the drawn area and has the draw routine draw the uncovered part; an
// unchanged position just redraws the selected row.
void opeu_list_scroll(Game& game, ListWindow const& window, std::uint8_t column, std::uint8_t row) {
  auto& data{ Data(game) };
  if (column < REDRAW_HERE && data.list_column == column && data.list_top == row) {
    RedrawSelection(game, window, column);
    return;
  }
  DrawRequest target{ column, row, 0, 0, 0 };
  auto const  step{ Shift(game, window, target) };
  data.list_column = target.column;
  data.list_top    = target.row;
  auto const saved{ DrawFrame(game, window, target.column) };
  auto const request{ Uncovered(data, step, saved, target) };
  if (!request) return;
  opeu_text_paper_set(game, static_cast<std::uint8_t>(4));
  DrawRows(game, window, *request);
  opeu_text_paper_set(game, static_cast<std::uint8_t>(0));
}
