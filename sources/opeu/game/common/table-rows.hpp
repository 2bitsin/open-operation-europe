// The row-draw callbacks of the list tables (core/tblgroup, core/tblpers): the services they call, the column layout
// they ask for, where a row goes on the screen and how a routine call runs them.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"

#include "common/game.hpp"
#include "port.hpp"

#include <cstdint>

namespace opeu::game::detail::table_rows {

// The services a row-draw callback calls through its event (svc_table, core/tblgroup).
enum TableService : std::uint16_t {
  COLUMNS      = 0, // (column, row, &lead, &start, &count): writes three DS bytes
  POSITION     = 1, // (x, y): the text cursor, x in columns
  FORMAT       = 2, // (destination, format, ...): formats into a DS buffer
  TEXT_COLOR   = 3, // (color)
  SIDE_COLOR   = 4, // (side): the side's text color
  UI_TEXT      = 5, // (text)
  PRINT        = 6, // (format, ...): prints at the cursor
  PUT_RANGE    = 7, // (text, start, count): prints the visible part of a row
  REQUEST_TEXT = 8, // (message id): the message's text, a DS offset
};

// The color the rows are printed in and restored to.
inline constexpr std::uint16_t TABLE_TEXT_COLOR{ 7 };

// Pixel rows per table row, and the offset of the first row below the window's top.
inline constexpr std::uint16_t TABLE_ROW_HEIGHT{ 17 };
inline constexpr std::uint16_t TABLE_FIRST_ROW{ 18 };

// The column layout of a table column: its text column offset and the visible character range.
struct TableColumns {
  std::uint8_t lead;
  std::uint8_t start;
  std::uint8_t count;
};

// Asks the layout service for a column of the active list window; the service writes the three bytes into DS locals.
inline auto Columns(Game& game, HostEvent event, std::uint8_t column, std::uint8_t row) -> TableColumns {
  ServiceLocals const bytes{ 4 };
  Call(game, event, COLUMNS, column, row, bytes.At(0), bytes.At(1), bytes.At(2));
  auto const& data{ Data(game) };
  return { Record<std::uint8_t>(data, bytes.At(0)), Record<std::uint8_t>(data, bytes.At(1)), Record<std::uint8_t>(data, bytes.At(2)) };
}

// The active list window.
[[nodiscard]] inline auto ActiveWindow(DataSegment const& data) -> ListWindow const& {
  return Record<ListWindow>(data, data.list_window);
}

// The text column where a table column starts: the window's left edge and the row width byte, plus the column's lead.
[[nodiscard]] inline auto ColumnX(DataSegment const& data, ListWindow const& window, TableColumns const& columns, int adjust) -> std::uint16_t {
  return static_cast<std::uint16_t>(Record<std::uint8_t>(data, window.widths) + columns.lead + window.x + adjust);
}

// The pixel row of table row `row` of the window, counted from the top visible row.
[[nodiscard]] inline auto RowY(DataSegment const& data, ListWindow const& window, std::uint8_t row) -> std::uint16_t {
  return static_cast<std::uint16_t>((row - data.list_top) * TABLE_ROW_HEIGHT + window.y + TABLE_FIRST_ROW);
}

// Runs a row-draw callback as a linked routine: its five byte arguments (column, first, row, last, flags) from the
// argument words.
template <typename Draw>
inline auto RunTableRows(Game& game, HostEvent event, Draw draw, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  auto const byte{ [&](unsigned i) { return static_cast<std::uint8_t>(Arg(args, count, i)); } };
  draw(game, event, byte(0), byte(1), byte(2), byte(3), byte(4));
  return 0;
}
} // namespace opeu::game::detail::table_rows

namespace opeu::game {
using detail::table_rows::ActiveWindow;
using detail::table_rows::Columns;
using detail::table_rows::ColumnX;
using detail::table_rows::RowY;
using detail::table_rows::RunTableRows;
using detail::table_rows::TABLE_FIRST_ROW;
using detail::table_rows::TABLE_ROW_HEIGHT;
using detail::table_rows::TABLE_TEXT_COLOR;
using detail::table_rows::TableColumns;
using detail::table_rows::TableService;
} // namespace opeu::game
