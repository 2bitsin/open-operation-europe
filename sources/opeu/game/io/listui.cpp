// List windows. A ListWindow has its left column (8-pixel units), top, right column and bottom, its width bytes (each
// 0x20 more than the width: the row width, then one per column, 0-terminated), the row count and far row-draw and
// select routines. list_window is the active window; list_column and list_top the first visible column and row,
// list_columns and list_rows the visible counts and list_row the selected row. list_mouse is set while the mouse
// scroll arrows show: a 16-pixel strip at the window's left edge with 32-pixel up and down arrows.
#include "io/listui.hpp"

#include "common/data-segment.hpp"
#include "common/host.hpp"
#include "core/limits.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/keyboard.hpp"
#include "io/list-keys.hpp"
#include "io/mouse.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The arrow tiles (data/fg3885): far segment and offsets.
constexpr std::uint16_t ARROW_SEGMENT{ 0x3885 };
constexpr std::uint16_t ARROW_UP{ 0x00 };
constexpr std::uint16_t ARROW_UP_LIT{ 0x40 };
constexpr std::uint16_t ARROW_DOWN{ 0x80 };
constexpr std::uint16_t ARROW_DOWN_LIT{ 0xc0 };

constexpr std::uint8_t  UP_LIT{ 0 };
constexpr std::uint8_t  DOWN_LIT{ 1 };
constexpr std::uint8_t  NONE_LIT{ 0xff };
constexpr std::uint8_t  ON_ROWS{ 2 };
constexpr std::uint16_t ROW_HEIGHT{ 0x11 };
constexpr std::uint8_t  WIDTH_BIAS{ 0x20 };
constexpr std::uint16_t COLOUR{ 7 };
constexpr std::uint16_t INVERT_MODE{ 0x107 };

// scroll: redraw from the origin, or at the current position
constexpr std::uint8_t REDRAW_ORIGIN{ 0xff };
constexpr std::uint8_t REDRAW_HERE{ 0xfe };

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

constexpr std::uint16_t SCRATCH_SIZE{ 16 }; // the keyboard prompts' scratch area

auto Window(DataSegment& data, std::uint16_t near) -> ListWindow& {
  return Record<ListWindow>(data, near);
}

// Width byte i of a window.
auto WidthByte(std::uint8_t* ds, ListWindow const& window, unsigned i) -> std::uint8_t& {
  return ds[static_cast<std::uint16_t>(window.widths + i)];
}

auto SetStream(DataSegment& data, std::uint16_t offset) -> void {
  data.stream_ptr[0] = offset;
  data.stream_ptr[1] = ARROW_SEGMENT;
}

auto DrawRows(std::uint8_t* ds, HostWordsEvent event, ListWindow const& window, std::uint8_t column, std::uint8_t row,
              std::uint8_t last_column, std::uint8_t last_row, std::uint8_t area) -> void {
  List(ds, event, ListCall::DRAW_ROWS, window.draw.offset, window.draw.segment, column, row, last_column, last_row,
       area);
}
} // namespace

// Draws the up arrow (lit when lit is UP_LIT) and the down arrow (lit when DOWN_LIT) of the scroll strip at pixel
// column x.
void opeu_list_arrows(std::uint8_t* ds, HostWordsEvent event, std::uint16_t x, std::uint16_t top, std::uint16_t bottom,
                      std::uint8_t lit) {
  auto& data{ Data(ds) };
  SetStream(data, lit == UP_LIT ? ARROW_UP_LIT : ARROW_UP);
  List(ds, event, ListCall::DRAW_FAR, x >> 3, top, 2, 0x20, 7, data.stream_ptr[1], data.stream_ptr[0]);
  SetStream(data, lit == DOWN_LIT ? ARROW_DOWN_LIT : ARROW_DOWN);
  List(ds, event, ListCall::DRAW_FAR, x >> 3, static_cast<std::uint16_t>(bottom - 0x1f), 2, 0x20, 7, data.stream_ptr[1],
       data.stream_ptr[0]);
}

// Outlines the scroll strip and draws both arrows unlit.
void opeu_list_scrollbar(std::uint8_t* ds, HostWordsEvent event, std::uint16_t x, std::uint16_t top,
                         std::uint16_t bottom) {
  if (Data(ds).list_mouse) {
    List(ds, event, ListCall::OUTLINE, x, top, static_cast<std::uint16_t>(x + 15), bottom, COLOUR);
    opeu_list_arrows(ds, event, x, top, bottom, NONE_LIT);
  }
}

// Stores at the DS byte `first` the offset (8-pixel units, from 2) of column `column` from the first visible column,
// at `second` and `third` the total widths of the columns 0..rows before and from `column`.
void opeu_table_columns(std::uint8_t* ds, HostWordsEvent, std::uint8_t column, std::uint8_t rows, std::uint16_t first,
                        std::uint16_t second, std::uint16_t third) {
  auto&       data{ Data(ds) };
  auto const& window{ Window(data, data.list_window) };
  ds[first] = 2;
  std::uint8_t i;
  for (i = data.list_column; i < column; i++) ds[first] += WidthByte(ds, window, i + 1) - WIDTH_BIAS;
  ds[second] = ds[third] = i = 0;
  for (; i <= rows; i++) {
    if (i < column)
      ds[second] += WidthByte(ds, window, i + 1) - WIDTH_BIAS;
    else
      ds[third] += WidthByte(ds, window, i + 1) - WIDTH_BIAS;
  }
}

// Redraws the arrows with the mouse cursor hidden.
void opeu_list_arrows_lit(std::uint8_t* ds, HostWordsEvent event, std::uint16_t x, std::uint16_t top,
                          std::uint16_t bottom, std::uint8_t lit) {
  if (Data(ds).list_mouse) {
    List(ds, event, ListCall::MOUSE_HIDE);
    opeu_list_arrows(ds, event, x, top, bottom, lit);
    List(ds, event, ListCall::MOUSE_SHOW);
  }
}

// Tracks the mouse over the active window until a button press; returns KEY_SCROLL_UP or KEY_SCROLL_DOWN (left click
// on an arrow), KEY_ENTER (left click on the selected row), KEY_ZERO (right click), or KEY_UP or KEY_DOWN when the
// pointer is over a row above or below the selection.
auto opeu_list_mouse_key(std::uint8_t* ds, HostWordsEvent event) -> std::uint8_t {
  auto&               data{ Data(ds) };
  auto const&         window{ Window(data, data.list_window) };
  std::uint16_t const left = window.x << 3;
  std::uint16_t const top{ window.y };
  std::uint16_t const bottom{ window.bottom };
  std::uint8_t        state{ NONE_LIT };
  std::uint8_t        key;
  List(ds, event, ListCall::MOUSE_SHOW);
  for (;;) {
    std::uint16_t position[2]{};
    event(ds, std::to_underlying(ListCall::MOUSE_POSITION), position, 2);
    auto const x{ position[0] };
    auto const y{ position[1] };
    bool const on_strip{ data.list_mouse && x >= left && static_cast<std::uint16_t>(left + 15) >= x };
    if (on_strip && y >= top && static_cast<std::uint16_t>(top + 0x1f) >= y) {
      if (state != UP_LIT) {
        opeu_list_arrows_lit(ds, event, left, top, bottom, UP_LIT);
        state = UP_LIT;
      }
    } else if (on_strip && static_cast<std::uint16_t>(bottom - 0x1f) <= y && y <= bottom) {
      if (state != DOWN_LIT) {
        opeu_list_arrows_lit(ds, event, left, top, bottom, DOWN_LIT);
        state = DOWN_LIT;
      }
    } else {
      if (state == UP_LIT || state == DOWN_LIT) opeu_list_arrows_lit(ds, event, left, top, bottom, NONE_LIT);
      state = NONE_LIT;
      auto const rows_bottom{ opeu_min16(static_cast<std::uint16_t>(window.rows * ROW_HEIGHT + top), bottom) };
      if (static_cast<std::uint16_t>(left + 16) <= x
          && static_cast<std::uint16_t>((WidthByte(ds, window, 0) << 3) + left - 0xf1) >= x
          && static_cast<std::uint16_t>(top + ROW_HEIGHT) <= y && rows_bottom >= y) {
        std::uint16_t const row_y = (data.list_row - data.list_top + 1) * ROW_HEIGHT + top;
        if (row_y > y) {
          key = KEY_UP;
          break;
        }
        if (static_cast<std::uint16_t>(row_y + ROW_HEIGHT) < y) {
          key = KEY_DOWN;
          break;
        }
        state = ON_ROWS;
      }
    }
    key = static_cast<std::uint8_t>(List(ds, event, ListCall::MOUSE_BUTTONS));
    if (key != 0) {
      if (key & 1) {
        switch (state) {
        case UP_LIT:   key = KEY_SCROLL_UP; break;
        case DOWN_LIT: key = KEY_SCROLL_DOWN; break;
        case ON_ROWS:  key = KEY_ENTER; break;
        }
        break;
      }
      if (key & 2) {
        key = KEY_ZERO;
        break;
      }
    }
  }
  List(ds, event, ListCall::MOUSE_HIDE);
  return key;
}

// Inverts the selected row.
void opeu_list_invert(std::uint8_t* ds, HostWordsEvent event) {
  auto&       data{ Data(ds) };
  auto const& window{ Window(data, data.list_window) };
  List(ds, event, ListCall::HIGHLIGHT, static_cast<std::uint16_t>(window.x + 2),
       static_cast<std::uint16_t>((data.list_row - data.list_top + 1) * 17 + window.y),
       WidthByte(ds, window, 0) - WIDTH_BIAS, 0x12, INVERT_MODE);
}

// Draws the window grid from first_column: the scroll strip, the outline, a line under each row that fits (setting
// list_rows) and the column separators (setting list_columns). Returns the previous visible column count.
auto opeu_list_frame(std::uint8_t* ds, HostWordsEvent event, std::uint16_t near, std::uint8_t first_column)
    -> std::uint8_t {
  auto&               data{ Data(ds) };
  auto const&         window{ Window(data, near) };
  auto const          saved{ data.list_columns };
  std::uint16_t       width{ window.widths };
  std::uint16_t       left  = window.x << 3;
  std::uint16_t const right = window.right << 3;
  std::uint16_t const top{ window.y };
  std::uint16_t const bottom{ window.bottom };
  opeu_list_scrollbar(ds, event, left, top, bottom);
  left                   += 0x10;
  std::uint16_t column_x  = ((ds[width++] - WIDTH_BIAS) << 3) + left;
  std::uint16_t line_y    = top + ROW_HEIGHT;
  List(ds, event, ListCall::OUTLINE, left, top, right, bottom, COLOUR);
  for (data.list_rows = 0; line_y <= bottom; data.list_rows++, line_y += ROW_HEIGHT) {
    if (window.rows > data.list_rows)
      List(ds, event, ListCall::LINE, left, line_y, right, line_y, COLOUR);
    else
      break;
  }
  data.list_rows--;
  width += first_column;
  for (data.list_columns = 0; column_x <= right; column_x += (ds[width++] - WIDTH_BIAS) << 3) {
    List(ds, event, ListCall::LINE, column_x, top, column_x, bottom, COLOUR);
    data.list_columns++;
    if (ds[width] == 0) break;
  }
  data.list_columns--;
  return saved;
}

// Shows the window from column and row. REDRAW_ORIGIN redraws from the origin, REDRAW_HERE at the current position;
// otherwise a one-column or one-row step scrolls the drawn area and has the draw routine draw the uncovered part; an
// unchanged position just redraws the selected row.
void opeu_list_scroll(std::uint8_t* ds, HostWordsEvent event, std::uint16_t near, std::uint8_t column,
                      std::uint8_t row) {
  auto&               data{ Data(ds) };
  auto const&         window{ Window(data, near) };
  std::uint16_t const width{ window.widths };
  std::uint16_t const x = window.x + 2;
  std::uint16_t const y{ window.y };
  std::uint16_t const x1{ window.right };
  std::uint16_t const y1{ window.bottom };
  std::uint8_t        area;
  if (column >= REDRAW_HERE) {
    if (column == REDRAW_ORIGIN) {
      column = row = 0;
    } else {
      column = data.list_column;
      row    = data.list_top;
    }
    area = ALL;
    if (data.list_mouse)
      List(ds, event, ListCall::BOX, x - 2, y, x1 - x + 2, y1 - y, 0);
    else
      List(ds, event, ListCall::BOX, x, y, x1 - x, y1 - y, 0);
  } else if (data.list_column < column) {
    area                     = SCROLL_LEFT;
    std::uint16_t const to   = ds[width] + x - WIDTH_BIAS;
    std::uint16_t const from = to + ds[static_cast<std::uint16_t>(width + column)] - WIDTH_BIAS;
    std::uint16_t const kept = x1 - from;
    List(ds, event, ListCall::SCREEN_COPY, from, y, kept, y1 - y, to, y);
    List(ds, event, ListCall::BOX, kept + to, y, x1 - kept - to, y1 - y, 0);
  } else if (data.list_column > column) {
    area                     = SCROLL_RIGHT;
    std::uint16_t const from = ds[width] + x - WIDTH_BIAS;
    std::uint16_t const to   = from + ds[static_cast<std::uint16_t>(width + column + 1)] - WIDTH_BIAS;
    std::uint16_t const kept = x1 - to;
    List(ds, event, ListCall::SCREEN_COPY, from, y, kept, y1 - y, to, y);
    List(ds, event, ListCall::BOX, from, y, ds[static_cast<std::uint16_t>(width + column + 1)] - WIDTH_BIAS, y1 - y, 0);
  } else if (data.list_top < row) {
    area = SCROLL_UP;
    List(ds, event, ListCall::SCREEN_COPY, x, y + 0x23, x1 - x, y1 - y - 0x23, x, y + 0x12);
    List(ds, event, ListCall::BOX, x, y1 - 0x10, x1 - x, 0x10, 0);
  } else if (data.list_top > row) {
    area = SCROLL_DOWN;
    List(ds, event, ListCall::SCREEN_COPY, x, y + 0x12, x1 - x, y1 - y - 0x23, x, y + 0x23);
    List(ds, event, ListCall::BOX, x, y + 0x12, x1 - x, 0x10, 0);
  } else {
    DrawRows(ds, event, window, column, data.list_row, data.list_columns + column - 1, data.list_row, SELECTED_ROW);
    opeu_list_frame(ds, event, near, column);
    return;
  }
  data.list_column = column;
  data.list_top    = row;
  auto const   saved{ opeu_list_frame(ds, event, near, column) };
  std::uint8_t last_column = data.list_columns + column - 1;
  std::uint8_t last_row    = data.list_rows + row - 1;
  switch (area) {
  case SCROLL_LEFT:
    if (data.list_columns < saved) return;
    area   = COLUMNS;
    column = saved - data.list_columns + last_column;
    break;
  case SCROLL_RIGHT:
    area        = COLUMNS;
    last_column = column;
    break;
  case SCROLL_UP:
    area = ROWS;
    row  = last_row;
    break;
  case SCROLL_DOWN:
    area     = ROWS;
    last_row = row;
    break;
  }
  List(ds, event, ListCall::PAPER, 4);
  DrawRows(ds, event, window, column, row, last_column, last_row, area);
  List(ds, event, ListCall::PAPER, 0);
}

auto svc_listui(std::uint8_t* ds, std::uint16_t kind, std::uint16_t* args, std::uint16_t count) -> std::uint16_t {
  auto* const text{ static_cast<TextHost*>(port_textout_host()) };
  auto const  word{ [&](unsigned i) { return Arg(args, count, i); } };
  switch (static_cast<ListCall>(kind)) {
  case ListCall::DRAW_FAR:
    return static_cast<std::uint16_t>(
        opeu_draw_far(word(0), word(1), word(2), word(3), word(4), port_far(word(5), word(6))));
  case ListCall::OUTLINE:
    opeu_ui_outline(ds, static_cast<UiHost const*>(port_uiprim_host()), word(0), word(1), word(2), word(3), word(4));
    break;
  case ListCall::MOUSE_SHOW:     opeu_mouse_show(ds, svc_mouse); break;
  case ListCall::MOUSE_HIDE:     opeu_mouse_hide(ds, svc_mouse); break;
  case ListCall::MOUSE_POSITION: {
    auto const at{ b1_stack_alloc(4) };
    opeu_mouse_position(ds, svc_mouse, at, static_cast<std::uint16_t>(at + 2));
    if (count >= 2) {
      args[0] = port_ds_word(at);
      args[1] = port_ds_word(static_cast<std::uint16_t>(at + 2));
    }
    port_scratch_leave(4);
    break;
  }
  case ListCall::MOUSE_BUTTONS: return opeu_mouse_buttons(ds, svc_mouse);
  case ListCall::INPUT_RESET:   opeu_input_reset(ds, svc_mouse); break;
  case ListCall::HIGHLIGHT:
    return static_cast<std::uint16_t>(opeu_ui_highlight(word(0), word(1), word(2), word(3), word(4)));
  case ListCall::LINE:        opeu_ui_line(word(0), word(1), word(2), word(3), word(4)); break;
  case ListCall::SCREEN_COPY: opeu_screen_copy(word(0), word(1), word(2), word(3), word(4), word(5)); break;
  case ListCall::BOX: return static_cast<std::uint16_t>(opeu_ui_box(word(0), word(1), word(2), word(3), word(4)));
  case ListCall::BITMAP_SAVE: opeu_bitmap_save(word(0), word(1), word(2), word(3), port_far(word(4), word(5))); break;
  case ListCall::BITMAP_RESTORE:
    return static_cast<std::uint16_t>(
        opeu_bitmap_restore(word(0), word(1), word(2), word(3), word(4), port_far(word(5), word(6))));
  case ListCall::PAPER:       opeu_text_paper_set(ds, text, static_cast<std::uint8_t>(word(0))); break;
  case ListCall::TEXT_COLOUR: opeu_text_color(ds, text, word(0)); break;
  case ListCall::TEXT_POSITION:
    opeu_army_position(ds, text, static_cast<std::int16_t>(word(0)), static_cast<std::int16_t>(word(1)));
    break;
  case ListCall::PRINT:        b1_cursor_print(ds, args, count); break;
  case ListCall::KEYBOARD_KEY: return opeu_keyboard_key(ds, svc_keyboard);
  case ListCall::YES_NO:       {
    auto const scratch{ b1_stack_alloc(SCRATCH_SIZE) };
    auto const answer{ static_cast<std::uint16_t>(opeu_prompt_yes_no(ds, svc_keyboard, word(0), scratch)) };
    port_scratch_leave(SCRATCH_SIZE);
    return answer;
  }
  case ListCall::DRAW_ROWS:
    port_call(ds, static_cast<std::uint32_t>(word(1)) * 16u + word(0), args + 2, count >= 2 ? count - 2u : 0u);
    break;
  case ListCall::SELECT:
    return static_cast<std::uint8_t>(
        port_call(ds, static_cast<std::uint32_t>(word(1)) * 16u + word(0), args + 2, count >= 2 ? count - 2u : 0u));
  default: port_unhandled("listui", kind, count);
  }
  return 0;
}

auto port_routine_opeu_table_columns(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_table_columns(ds, svc_listui, static_cast<std::uint8_t>(Arg(args, count, 0)),
                     static_cast<std::uint8_t>(Arg(args, count, 1)), Arg(args, count, 2), Arg(args, count, 3),
                     Arg(args, count, 4));
  return 0;
}
