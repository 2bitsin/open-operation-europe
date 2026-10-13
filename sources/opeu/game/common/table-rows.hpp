// The row-draw callbacks of the list tables (core/tblgroup, core/tblpers): the column layout they ask for, where a row
// goes on the screen.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"

#include "common/game.hpp"
#include "port.hpp"

#include <cstdint>

namespace opeu::game::detail::table_rows {

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
} // namespace opeu::game::detail::table_rows

namespace opeu::game {
using detail::table_rows::ActiveWindow;
using detail::table_rows::ColumnX;
using detail::table_rows::RowY;
using detail::table_rows::TABLE_FIRST_ROW;
using detail::table_rows::TABLE_ROW_HEIGHT;
using detail::table_rows::TABLE_TEXT_COLOR;
using detail::table_rows::TableColumns;
} // namespace opeu::game

// The layout of table column `column` of the active list window (io/listui.cpp).
auto TableColumnLayout(Game& game, std::uint8_t column, std::uint8_t rows) -> opeu::game::TableColumns;
