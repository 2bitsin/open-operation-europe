// What the regiment, element, type, supply and strength tables (units/tbl*) share: the heading, the printed part of a
// row and the start of a row's name.
#pragma once

#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "link/glue.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <ranges>

namespace opeu::game::detail::unit_table {

// Bits of a row-draw callback's flags: draw the heading, draw the row names.
inline constexpr std::uint8_t DRAW_HEADING{ 3 };
inline constexpr std::uint8_t DRAW_NAMES{ 5 };
// The bytes of a callback's buffers (the column layout bytes, the row text and its other texts), which it reserves
// itself: the services format and print them in the data segment.
inline constexpr std::uint16_t TABLE_SCRATCH{ 0xc0 };
// The row text buffer at the start of the frame buffers.
inline constexpr std::uint16_t ROW_TEXT{ 4 };

// The rows `first` to `last` a row-draw callback draws, none when `first` is past `last`.
[[nodiscard]] inline auto DrawnRows(std::uint8_t first, std::uint8_t last) {
  return std::views::iota(unsigned{ first }, std::max(unsigned{ first }, last + 1u)) | std::views::transform([](unsigned row) { return static_cast<std::uint8_t>(row); });
}

// The text column of the table body's left edge (the column's lead within the window).
[[nodiscard]] inline auto BodyX(DataSegment const& data, ListWindow const& window, TableColumns const& layout) -> std::uint16_t {
  return ColumnX(data, window, layout, -0x20);
}

// Formats the heading `format` into `text` and prints the column's part of it above the rows.
inline auto DrawHeading(Game& game, ListWindow const& window, TableColumns const& layout, std::uint16_t text, std::uint16_t format) -> void {
  auto const& data{ Data(game) };
  opeu_request_format(game, std::array<std::uint16_t, 2>{ text, format });
  opeu_army_position(game, static_cast<int16_t>(BodyX(data, window, layout)), static_cast<int16_t>(window.y + 1));
  opeu_select_side_color(game, static_cast<uint8_t>(data.current_side));
  opeu_text_put_range(game, text, static_cast<uint8_t>(layout.start), static_cast<uint8_t>(layout.count));
}

// Prints the column's part of the row text at row `entry`.
inline auto DrawRowText(Game& game, ListWindow const& window, TableColumns const& layout, std::uint8_t entry, std::uint16_t text) -> void {
  auto const& data{ Data(game) };
  opeu_army_position(game, static_cast<int16_t>(BodyX(data, window, layout)), static_cast<int16_t>(RowY(data, window, entry)));
  opeu_text_color(game, TABLE_TEXT_COLOR);
  opeu_text_put_range(game, text, static_cast<uint8_t>(layout.start), static_cast<uint8_t>(layout.count));
}

// Moves the text cursor to the name of row `entry`, `indent` columns into the window, in the colour of `side`.
inline auto StartRowName(Game& game, ListWindow const& window, std::uint8_t entry, std::uint16_t indent, std::uint8_t side) -> void {
  auto const& data{ Data(game) };
  opeu_army_position(game, static_cast<int16_t>(window.x + indent), static_cast<int16_t>(RowY(data, window, entry)));
  opeu_select_side_color(game, static_cast<uint8_t>(side));
}

} // namespace opeu::game::detail::unit_table

namespace opeu::game {
using detail::unit_table::DRAW_HEADING;
using detail::unit_table::DRAW_NAMES;
using detail::unit_table::DrawnRows;
using detail::unit_table::DrawHeading;
using detail::unit_table::DrawRowText;
using detail::unit_table::ROW_TEXT;
using detail::unit_table::StartRowName;
using detail::unit_table::TABLE_SCRATCH;
} // namespace opeu::game
