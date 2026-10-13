// The row-draw and choice callbacks of the saved-game list ("Scenario Dfclty Date(1939-45) Axis Allies"). Rows
// first..last are the save slots; an empty slot shows only its number. The scenario and difficulty names are kept in
// tbl_save_map and tbl_save_level, which keep the previous row's names when a code is out of range; the side controls
// index the Computer/User names.
#include "io/tblsave.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/saved-game.hpp"
#include "common/scenario.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "io/listui.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

enum Service : std::uint16_t {
  COLUMNS   = 0, // (column, row, out0, out1, out2): writes the three DS bytes
  POSITION  = 1, // (x, y)
  FORMAT    = 2, // (destination, format, args...)
  PRINT     = 6, // (format, args...)
  PUT_RANGE = 7, // (text, start, count)
};

constexpr std::uint8_t  DRAW_HEADER{ 3 }; // flags bits that draw the header
constexpr std::uint8_t  DRAW_NUMBER{ 5 }; // flags bits that draw the slot numbers
constexpr std::uint16_t ROW_HEIGHT{ 17 };
constexpr std::uint16_t BUFFER_OFFSET{ 4 }; // the text buffer in the scratch area, after the column bytes
constexpr std::uint16_t SCRATCH_SIZE{ 0x68 };
} // namespace

// The choice callback: always accepts.
auto opeu_tbl_save_accept() -> std::uint8_t {
  return 1;
}

// Draws the saved-game table: slot number, scenario, difficulty, date and players. Its reserve holds the column
// layout's three bytes, then the text buffer.
void opeu_tbl_draw_saves(std::uint8_t* ds, HostEvent event, std::uint8_t column, std::uint8_t first, std::uint8_t row, std::uint8_t last, std::uint8_t flags) {
  ServiceLocals const locals{ SCRATCH_SIZE };
  locals.Clear(ds);
  auto&              data{ Data(ds) };
  auto const         scratch{ locals.At(0) };
  auto const         buffer{ static_cast<std::uint16_t>(scratch + BUFFER_OFFSET) };
  auto const&        window{ Record<ListWindow>(data, data.list_window) };
  std::uint8_t const row_width{ ds[window.widths] };
  Call(ds, event, COLUMNS, column, row, scratch, scratch + 1, scratch + 2);
  std::uint8_t const x{ ds[scratch] };
  std::uint8_t const start{ ds[static_cast<std::uint16_t>(scratch + 1)] };
  std::uint8_t const count{ ds[static_cast<std::uint16_t>(scratch + 2)] };
  auto const         text_x{ row_width + x + window.x - 0x20 };
  if (flags & DRAW_HEADER) {
    Call(ds, event, FORMAT, buffer, Near(data, data.tbl_save_header));
    Call(ds, event, POSITION, text_x, window.y + 1);
    Call(ds, event, PUT_RANGE, buffer, start, count);
  }
  for (std::uint8_t slot_index{ first }; slot_index <= last; ++slot_index) {
    auto const& slot{ data.save_slots[slot_index] };
    auto const  y{ (slot_index - data.list_top) * ROW_HEIGHT + window.y + 18 };
    if (flags & DRAW_NUMBER) {
      Call(ds, event, POSITION, window.x + 3, y);
      Call(ds, event, PRINT, Near(data, data.tbl_save_number), slot_index + 1);
    }
    switch (slot.scenario & SCENARIO_MASK) {
    case 0: data.tbl_save_map = Near(data, data.tbl_map_france); break;
    case 1: data.tbl_save_map = Near(data, data.tbl_map_africa); break;
    case 2: data.tbl_save_map = Near(data, data.tbl_map_kursk); break;
    case 3: data.tbl_save_map = Near(data, data.tbl_map_dday); break;
    case 4: data.tbl_save_map = Near(data, data.tbl_map_bulge); break;
    case 5: data.tbl_save_map = Near(data, data.tbl_map_berlin); break;
    }
    switch (slot.delivery) {
    case 0: data.tbl_save_level = Near(data, data.tbl_level_veasy); break;
    case 1: data.tbl_save_level = Near(data, data.tbl_level_easy); break;
    case 2: data.tbl_save_level = Near(data, data.tbl_level_avg); break;
    case 3: data.tbl_save_level = Near(data, data.tbl_level_hard); break;
    case 4: data.tbl_save_level = Near(data, data.tbl_level_vhard); break;
    }
    if ((slot.scenario & SCENARIO_MASK) != SLOT_EMPTY) {
      Call(ds, event, FORMAT, buffer, Near(data, data.tbl_save_row), data.tbl_save_map, data.tbl_save_level, slot.month, slot.day, slot.year, slot.hour, data.tbl_player_names[slot.control[0]],
           data.tbl_player_names[slot.control[1]]);
      Call(ds, event, POSITION, text_x, y);
      Call(ds, event, PUT_RANGE, buffer, start, count);
    }
  }
}

auto svc_tblsave(std::uint8_t* ds, std::uint16_t kind, std::uint16_t const* args, std::uint16_t count) -> std::uint16_t {
  switch (kind) {
  case COLUMNS:
    opeu_table_columns(ds, svc_listui, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4));
    break;
  case POSITION: opeu_army_position(ds, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(Arg(args, count, 0)), static_cast<std::int16_t>(Arg(args, count, 1))); break;
  case FORMAT:   opeu_request_format(ds, { args, count }); break;
  case PRINT:    opeu_cursor_print(ds, { args, count }); break;
  case PUT_RANGE:
    opeu_text_put_range(ds, static_cast<TextHost*>(port_textout_host()), Arg(args, count, 0), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)));
    break;
  default: port_unhandled("tblsave", kind, count);
  }
  return 0;
}

auto port_routine_opeu_tbl_draw_saves(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_tbl_draw_saves(ds, svc_tblsave, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1)), static_cast<std::uint8_t>(Arg(args, count, 2)),
                      static_cast<std::uint8_t>(Arg(args, count, 3)), static_cast<std::uint8_t>(Arg(args, count, 4)));
  return 0;
}

auto port_routine_opeu_tbl_save_accept(std::uint8_t*, std::uint16_t const*, unsigned) -> std::uint32_t {
  return opeu_tbl_save_accept();
}
