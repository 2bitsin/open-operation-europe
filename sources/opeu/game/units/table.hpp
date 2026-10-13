// What the regiment, element, type, supply and strength tables (units/tbl*) share: the heading, the printed part of a
// row and the start of a row's name.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"

#include <cstdint>

namespace opeu::game::detail::unit_table {

// Bits of a row-draw callback's flags: draw the heading, draw the row names.
inline constexpr std::uint8_t DRAW_HEADING{ 3 };
inline constexpr std::uint8_t DRAW_NAMES{ 5 };
// The bytes of a callback's buffers (the column layout bytes, the row text and its other texts), which it reserves
// itself: the services format and print them in the data segment.
inline constexpr std::uint16_t TABLE_SCRATCH{ 0xc0 };
// The row text buffer at the start of the frame buffers.
inline constexpr std::uint16_t ROW_TEXT{ 4 };

// The text column of the table body's left edge (the column's lead within the window).
[[nodiscard]] inline auto BodyX(DataSegment const& data, ListWindow const& window, TableColumns const& layout) -> std::uint16_t {
  return ColumnX(data, window, layout, -0x20);
}

// Formats the heading `format` into `text` and prints the column's part of it above the rows.
inline auto DrawHeading(std::uint8_t* ds, HostEvent event, ListWindow const& window, TableColumns const& layout, std::uint16_t text, std::uint16_t format) -> void {
  auto const& data{ Data(ds) };
  Call(ds, event, TableService::FORMAT, text, format);
  Call(ds, event, TableService::POSITION, BodyX(data, window, layout), window.y + 1);
  Call(ds, event, TableService::SIDE_COLOR, data.current_side);
  Call(ds, event, TableService::PUT_RANGE, text, layout.start, layout.count);
}

// Prints the column's part of the row text at row `entry`.
inline auto DrawRowText(std::uint8_t* ds, HostEvent event, ListWindow const& window, TableColumns const& layout, std::uint8_t entry, std::uint16_t text) -> void {
  auto const& data{ Data(ds) };
  Call(ds, event, TableService::POSITION, BodyX(data, window, layout), RowY(data, window, entry));
  Call(ds, event, TableService::TEXT_COLOR, TABLE_TEXT_COLOR);
  Call(ds, event, TableService::PUT_RANGE, text, layout.start, layout.count);
}

// Moves the text cursor to the name of row `entry`, `indent` columns into the window, in the colour of `side`.
inline auto StartRowName(std::uint8_t* ds, HostEvent event, ListWindow const& window, std::uint8_t entry, std::uint16_t indent, std::uint8_t side) -> void {
  auto const& data{ Data(ds) };
  Call(ds, event, TableService::POSITION, window.x + indent, RowY(data, window, entry));
  Call(ds, event, TableService::SIDE_COLOR, side);
}

} // namespace opeu::game::detail::unit_table

namespace opeu::game {
using detail::unit_table::DRAW_HEADING;
using detail::unit_table::DRAW_NAMES;
using detail::unit_table::DrawHeading;
using detail::unit_table::DrawRowText;
using detail::unit_table::ROW_TEXT;
using detail::unit_table::StartRowName;
using detail::unit_table::TABLE_SCRATCH;
} // namespace opeu::game
