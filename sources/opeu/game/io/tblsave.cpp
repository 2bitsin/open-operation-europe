// The row-draw and choice callbacks of the saved-game list ("Scenario Dfclty Date(1939-45) Axis Allies"). Rows
// first..last are the save slots; an empty slot shows only its number. The scenario and difficulty names are kept in
// tbl_save_map and tbl_save_level, which keep the previous row's names when a code is out of range; the side controls
// index the Computer/User names.
#include "io/tblsave.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/saved-game.hpp"
#include "common/scenario.hpp"
#include "core/textout.hpp"
#include "io/listui.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {

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
void opeu_tbl_draw_saves(Game& game, std::uint8_t column, std::uint8_t first, std::uint8_t row, std::uint8_t last, std::uint8_t flags) {
  ServiceLocals const locals{ SCRATCH_SIZE };
  locals.Clear(game);
  auto&              data{ Data(game) };
  auto const         scratch{ locals.At(0) };
  auto const         buffer{ static_cast<std::uint16_t>(scratch + BUFFER_OFFSET) };
  auto const&        window{ Record<ListWindow>(data, data.list_window) };
  std::uint8_t const row_width{ Byte(game, window.widths) };
  opeu_table_columns(game, static_cast<std::uint8_t>(column), static_cast<std::uint8_t>(row), scratch, scratch + 1, scratch + 2);
  std::uint8_t const x{ Byte(game, scratch) };
  std::uint8_t const start{ Byte(game, static_cast<std::uint16_t>(scratch + 1)) };
  std::uint8_t const count{ Byte(game, static_cast<std::uint16_t>(scratch + 2)) };
  auto const         text_x{ row_width + x + window.x - 0x20 };
  if (flags & DRAW_HEADER) {
    opeu_request_format(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(Near(data, data.tbl_save_header)) });
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(text_x), static_cast<std::int16_t>(window.y + 1));
    opeu_text_put_range(game, static_cast<TextHost*>(port_textout_host()), buffer, static_cast<std::uint8_t>(start), static_cast<std::uint8_t>(count));
  }
  for (std::uint8_t slot_index{ first }; slot_index <= last; ++slot_index) {
    auto const& slot{ data.save_slots[slot_index] };
    auto const  y{ (slot_index - data.list_top) * ROW_HEIGHT + window.y + 18 };
    if (flags & DRAW_NUMBER) {
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(window.x + 3), static_cast<std::int16_t>(y));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.tbl_save_number)), static_cast<std::uint16_t>(slot_index + 1) });
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
      opeu_request_format(game,
                          std::array<std::uint16_t, 10>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(Near(data, data.tbl_save_row)), static_cast<std::uint16_t>(data.tbl_save_map),
                                                         static_cast<std::uint16_t>(data.tbl_save_level), static_cast<std::uint16_t>(slot.month), static_cast<std::uint16_t>(slot.day),
                                                         static_cast<std::uint16_t>(slot.year), static_cast<std::uint16_t>(slot.hour),
                                                         static_cast<std::uint16_t>(data.tbl_player_names[slot.control[0]]), static_cast<std::uint16_t>(data.tbl_player_names[slot.control[1]]) });
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<std::int16_t>(text_x), static_cast<std::int16_t>(y));
      opeu_text_put_range(game, static_cast<TextHost*>(port_textout_host()), buffer, static_cast<std::uint8_t>(start), static_cast<std::uint8_t>(count));
    }
  }
}
