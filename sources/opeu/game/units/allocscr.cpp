// The equipment allocation screen: two unit columns, the type list with nation codes, and the per-type remaining and
// reserve amounts.
#include "units/allocscr.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "core/strings.hpp"
#include "types.hpp"
#include "units/typetext.hpp"

#include "common/game.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <algorithm>
#include <array>
#include <ranges>
#include <span>

using namespace opeu::game;

namespace {
// The buffer a type's nation code is written to.
constexpr std::uint16_t NATION_BYTES{ 0x1a };

// A unit name centred in its column heading, capped at ten characters.
[[nodiscard]] auto HeadingLength(Game& game, std::uint16_t name) -> std::uint16_t {
  return std::min<std::uint16_t>(opeu_string_length(game, name), 10);
}

// The height of `rows` rows of the screen, in pixels.
[[nodiscard]] constexpr auto RowsHeight(unsigned rows) -> std::uint16_t {
  return static_cast<std::uint16_t>(rows * 17);
}

auto Outline(Game& game, std::uint16_t left, std::uint16_t top, std::uint16_t bottom) -> void {
  constexpr std::uint16_t RIGHT{ 0x170 };
  opeu_ui_outline(game, left, top, RIGHT, bottom, 7);
}

auto DrawFirstColumn(Game& game, std::uint8_t rows) -> void {
  auto&      data{ Data(game) };
  auto const name{ Near(data, data.tally_first_name) };
  auto const length{ HeadingLength(game, name) };
  Outline(game, 0x10f, 0x2d, 0x3e);
  Outline(game, 0x15f, 0x2d, RowsHeight(rows) + 0x2d);
  opeu_vga_fill_rect(0x22, 0x2e, 10, 0x10, 3);
  opeu_vga_fill_rect(0x2c, 0x2e, 2, RowsHeight(rows), 3);
  opeu_army_position(game, static_cast<int16_t>(0x28 - (length >> 1)), 0x2e);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.alloc_first_format), name });
}

// The second column starts below the first one's `above` rows.
auto DrawSecondColumn(Game& game, std::uint8_t above, std::uint8_t rows) -> void {
  auto&      data{ Data(game) };
  auto const name{ Near(data, data.tally_second_name) };
  auto const length{ HeadingLength(game, name) };
  auto const top{ RowsHeight(above) };
  Outline(game, 0x10f, top + 0x2d, top + 0x3e);
  Outline(game, 0x15f, top + 0x2d, RowsHeight(above + rows) + 0x2d);
  opeu_vga_fill_rect(0x22, static_cast<std::uint16_t>(top + 0x2e), 10, 0x10, 5);
  opeu_vga_fill_rect(0x2c, static_cast<std::uint16_t>(top + 0x2e), 2, static_cast<std::uint16_t>(RowsHeight(rows) - 1), 5);
  opeu_army_position(game, static_cast<int16_t>(0x28 - (length >> 1)), static_cast<int16_t>(top + 0x2e));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.alloc_second_format), name });
}

// The type list: each type's name and nation code, written to the routine's 22-byte text buffer for printing.
auto DrawTypeList(Game& game, std::uint8_t count) -> void {
  auto&               data{ Data(game) };
  ServiceLocals const nation{ NATION_BYTES };
  auto const          buffer{ nation.At(0) };
  opeu_vga_fill_rect(2, 0x1c, 0x1e, 0xde, 0);
  opeu_ui_outline(game, 0x10, 0x1c, 0x100, 0xf9, 7);
  opeu_army_position(game, 3, 0x1d);
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ Near(data, data.alloc_heading) });
  for (auto const [row, supply_type] : std::views::zip(std::views::iota(0u), std::span{ data.supply_types }.first(count))) {
    auto const& type{ data.element_types[supply_type] };
    auto const  top{ RowsHeight(row) };
    opeu_army_position(game, 2, static_cast<int16_t>(top + 0x2e));
    opeu_cursor_print(game, std::array<std::uint16_t, 4>{ Near(data, data.alloc_row_format), SideTextColor(data.current_side), type.name, opeu_nation_code(game, Near(data, type), buffer) });
    opeu_ui_outline(game, 0x10, top + 0x2d, 0x100, top + 0x3e, 7);
  }
  opeu_ui_outline(game, 0x80, 0x1c, 0xa8, 0xf9, 7);
}
} // namespace

// Remaining and reserve amounts of each type, one row per type.
void opeu_alloc_amounts_show(Game& game, uint8_t count) {
  auto& data{ Data(game) };
  for (auto const [row, amount, other] : std::views::zip(std::views::iota(0u), std::span{ data.supply_amounts }.first(count), data.supply_others)) {
    opeu_army_position(game, 0x16, static_cast<int16_t>(RowsHeight(row) + 0x2e));
    opeu_cursor_print(game, std::array<std::uint16_t, 4>{ Near(data, data.alloc_amount_format), SideTextColor(data.current_side), amount, other });
  }
}

// Gives the amount of the type in the slot to the stock entry and takes it from the slot and the total.
void opeu_alloc_take(Game& game, Take const& take, uint8_t count) {
  auto& data{ Data(game) };
  auto& item{ Record<Item>(data, take.entry) };
  item.type                       = data.supply_types[take.slot];
  item.quantity                   = take.amount;
  data.supply_amounts[take.slot] -= take.amount;
  data.supply_total              -= take.amount;
  opeu_alloc_amounts_show(game, count);
}

// Draws both unit columns (an empty second column takes the spare rows), the names capped at ten characters, and the
// type list.
void opeu_alloc_screen(Game& game, Allocation const& allocation) {
  auto& data{ Data(game) };
  opeu_backdrop_fill_color(game, data.current_side ? 1 : 2);
  opeu_vga_fill_rect(0x2b, 0x110, 0x23, 0x70, 0);
  opeu_ui_outline(game, 0x158, 0x110, 0x26f, 0x17f, 7);
  DrawFirstColumn(game, data.supply_count);
  DrawSecondColumn(game, data.supply_count, allocation.other_slots ? allocation.other_slots : allocation.spare_slots);
  DrawTypeList(game, allocation.types);
  opeu_alloc_amounts_show(game, allocation.types);
}
