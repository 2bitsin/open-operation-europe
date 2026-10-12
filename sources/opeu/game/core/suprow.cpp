// Redraws one row of the supply-allocation panel for a side a player commands: the two amounts of the row in the
// supply table as text and their percentages of the capacities as digit glyphs over cleared boxes. Rows are 28 pixels
// apart; column is not used.
#include "core/suprow.hpp"
#include "core/suppanel.hpp"
#include "types.hpp"
#include "units/ratio.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

using enum SupplyService;

constexpr std::uint16_t ROW_HEIGHT{ 28 };
constexpr std::uint8_t  CURRENT{ 0 };  // supply_table column: the amount
constexpr std::uint8_t  CAPACITY{ 1 }; // and the capacity
} // namespace

void opeu_supply_draw_row(uint8_t ds[65536], HostEvent event, uint8_t row, uint8_t column) {
  (void)column;
  auto const& data{ Data(ds) };
  if (data.side_status[data.current_side] != SIDE_PLAYED) return;
  auto const left{ opeu_supply_get(ds, 0, row, CURRENT) };
  auto const right{ opeu_supply_get(ds, 1, row, CURRENT) };
  auto const left_capacity{ opeu_supply_get(ds, 0, row, CAPACITY) };
  auto const right_capacity{ opeu_supply_get(ds, 1, row, CAPACITY) };
  auto const left_percent{ opeu_resource_percent(left, left_capacity) };
  auto const right_percent{ opeu_resource_percent(right, right_capacity) };
  auto const x{ data.supply_panel_x };
  auto const top{ static_cast<std::uint16_t>(static_cast<std::uint16_t>(row) * ROW_HEIGHT + data.supply_panel_y) };
  auto const text_y{ static_cast<std::uint16_t>(top + 0x24) };
  auto const digits_y{ static_cast<std::uint16_t>(top + 0x37) };
  auto const fill{ [&](std::uint16_t fill_x) { Call(ds, event, FILL, fill_x, digits_y, 3, 6, 0); } };
  Call(ds, event, POSITION, x + 2, text_y);
  Call(ds, event, PRINT, offsetof(DataSegment, supply_row_left), left);
  Call(ds, event, POSITION, x + 25, text_y);
  Call(ds, event, PRINT, offsetof(DataSegment, supply_row_right), right);
  fill(x + 2);
  opeu_supply_digits(ds, event, (x + 4) << 3, digits_y, left_percent);
  fill(x + 25);
  opeu_supply_digits(ds, event, (x + 27) << 3, digits_y, right_percent);
  Call(ds, event, PRINT, offsetof(DataSegment, supply_row_end));
}

uint16_t svc_suprow(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (static_cast<SupplyService>(id)) {
  case POSITION: opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case PRINT:    opeu_cursor_print(ds, { args, count }); break;
  case GLYPH:    opeu_vga_glyph(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4), PORT_DS(Arg(args, count, 5))); break;
  case FILL:     opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  default:       port_unhandled("suprow", id, count);
  }
  return 0;
}
