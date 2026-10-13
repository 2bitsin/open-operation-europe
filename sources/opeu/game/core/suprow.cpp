// Redraws one row of the supply-allocation panel for a side a player commands: the two amounts of the row in the
// supply table as text and their percentages of the capacities as digit glyphs over cleared boxes. Rows are 28 pixels
// apart; column is not used.
#include "core/suprow.hpp"
#include "core/suppanel.hpp"
#include "types.hpp"
#include "units/ratio.hpp"

#include "common/data-segment.hpp"
#include "common/exchange-panel.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint8_t CURRENT{ 0 };  // supply_table column: the amount
constexpr std::uint8_t CAPACITY{ 1 }; // and the capacity
} // namespace

void opeu_supply_draw_row(Game& game, uint8_t row, uint8_t column) {
  (void)column;
  auto const& data{ Data(game) };
  if (data.side_status[data.current_side] != SIDE_PLAYED) return;
  auto const left{ opeu_supply_get(game, 0, row, CURRENT) };
  auto const right{ opeu_supply_get(game, 1, row, CURRENT) };
  auto const left_capacity{ opeu_supply_get(game, 0, row, CAPACITY) };
  auto const right_capacity{ opeu_supply_get(game, 1, row, CAPACITY) };
  auto const left_percent{ opeu_resource_percent(left, left_capacity) };
  auto const right_percent{ opeu_resource_percent(right, right_capacity) };
  auto const x{ data.supply_panel_x };
  auto const top{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(row) * PANEL_ROW_HEIGHT + data.supply_panel_y) };
  auto const text_y{ static_cast<std::uint16_t>(top + 0x24) };
  auto const digits_y{ static_cast<std::uint16_t>(top + 0x37) };
  auto const fill{ [&](std::uint16_t fill_x) { opeu_ui_box(fill_x, digits_y, 3, 6, 0); } };
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(x + 2), static_cast<int16_t>(text_y));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_row_left)), static_cast<std::uint16_t>(left) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(x + 25), static_cast<int16_t>(text_y));
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_row_right)), static_cast<std::uint16_t>(right) });
  fill(x + 2);
  opeu_supply_digits(game, (x + 4) << 3, digits_y, left_percent);
  fill(x + 25);
  opeu_supply_digits(game, (x + 27) << 3, digits_y, right_percent);
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(offsetof(DataSegment, supply_row_end)) });
}
