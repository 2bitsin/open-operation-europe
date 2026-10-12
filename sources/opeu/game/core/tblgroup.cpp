// The row-draw callbacks of the division table and the mission table, and the service table every row-draw callback
// calls through. A callback gets (column, first, row, last, flags): flags & 3 draws the header line, flags & 5 the row
// labels, and rows first..last are formatted into a frame buffer and printed through the column's visible range.
#include "core/tblgroup.hpp"
#include "core/people.hpp"
#include "core/selunit.hpp"
#include "core/strings.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/aggpct.hpp"

#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/msglook.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "glue.hpp"
#include "io/listui.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

using enum TableService;

constexpr std::uint8_t DRAW_HEADER{ 3 }; // flags that draw the header line
constexpr std::uint8_t DRAW_LABELS{ 5 }; // flags that draw the row labels

} // namespace

// The division table: each division's label and, for a division with an eligible selected unit, its commander and
// its fuel, ammunition and food percentages.
void opeu_tbl_draw_groups(uint8_t ds[65536], HostEvent event, uint16_t scratch, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&       data{ Data(ds) };
  auto const  buffer{ static_cast<std::uint16_t>(scratch + 4) };
  auto const& window{ ActiveWindow(data) };
  auto const  columns{ Columns(ds, event, scratch, column, row) };
  auto const  side{ data.current_side };
  if (flags & DRAW_HEADER) {
    Call(ds, event, FORMAT, buffer, Near(data, data.tbl_group_header));
    Call(ds, event, POSITION, ColumnX(data, window, columns, -0x20), window.y + 1);
    Call(ds, event, SIDE_COLOR, side);
    Call(ds, event, PRINT, Near(data, data.tbl_group_title), buffer);
  }
  for (std::uint8_t g = first; g <= last; ++g) {
    auto const& group{ GroupOf(data, side, g) };
    if (flags & DRAW_LABELS) {
      Call(ds, event, POSITION, window.x + 2, RowY(data, window, g));
      Call(ds, event, SIDE_COLOR, side);
      if (group.flags & Bits(GroupFlag::READY)) {
        if (group.flags & Bits(GroupFlag::SELECTED)) Call(ds, event, PRINT, Near(data, data.tbl_group_mark));
        Call(ds, event, PRINT, Near(data, data.tbl_group_division), g + 1);
      } else
        Call(ds, event, UI_TEXT, Near(data, data.tbl_group_empty));
    }
    auto const group_near{ Near(data, group) };
    if ((group.flags & Bits(GroupFlag::READY)) && opeu_unit_selection_eligible(ds, opeu_find_group_selected_unit(ds, group_near))) {
      auto const    commander{ opeu_find_group_record(ds, side, g) };
      std::uint16_t ammo;
      std::uint16_t fuel;
      std::uint16_t food;
      opeu_group_resource_percentages(ds, group_near, &ammo, &fuel, &food);
      auto const kind{ (group.flags & Bits(GroupFlag::AI_MARK)) ? Near(data, data.tbl_group_div) : Near(data, data.tbl_group_army) };
      Call(ds, event, FORMAT, buffer, Near(data, data.tbl_group_row), kind, commander, fuel, ammo, food);
    } else
      opeu_string_copy(ds, buffer, Near(data, data.tbl_group_blank));
    Call(ds, event, POSITION, ColumnX(data, window, columns, -0x20), RowY(data, window, g));
    Call(ds, event, TEXT_COLOR, TABLE_TEXT_COLOR);
    Call(ds, event, PUT_RANGE, buffer, columns.start, columns.count);
  }
  Call(ds, event, TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The mission table of the current division: each of its four tasks shows its order and the objective at the task's
// cell, or the cell in map coordinates.
void opeu_tbl_draw_missions(uint8_t ds[65536], HostEvent event, uint16_t scratch, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&       data{ Data(ds) };
  auto const  buffer{ static_cast<std::uint16_t>(scratch + 4) };
  auto const  place{ static_cast<std::uint16_t>(scratch + 64) };
  auto const& window{ ActiveWindow(data) };
  auto const  columns{ Columns(ds, event, scratch, column, row) };
  auto const  side{ data.current_side };
  if (flags & DRAW_HEADER) {
    Call(ds, event, POSITION, 0x26, 0x81);
    Call(ds, event, SIDE_COLOR, side);
    Call(ds, event, PRINT, Near(data, data.tbl_mission_header));
  }
  auto const& tasks{ GroupOf(data, side, data.current_group).tasks };
  for (std::uint8_t m = first; m <= last; ++m) {
    auto const& task{ tasks[m] };
    if (flags & DRAW_LABELS) {
      Call(ds, event, SIDE_COLOR, side);
      Call(ds, event, POSITION, window.x + 2, RowY(data, window, m));
      Call(ds, event, PRINT, Near(data, data.tbl_mission_label), m + 1);
    }
    if (task.kind != Bits(TaskKind::NONE)) {
      auto const x{ task.x };
      auto const y{ task.y };
      if (auto const objective{ opeu_find_objective_at(ds, x, y) })
        Call(ds, event, FORMAT, place, Near(data, data.tbl_mission_target), objective);
      else
        Call(ds, event, FORMAT, place, Near(data, data.tbl_mission_place), x * data.map_scale, y * data.map_scale);
      Call(ds, event, FORMAT, buffer, Near(data, data.tbl_mission_row), data.order_names[task.kind], place);
    } else
      opeu_string_copy(ds, buffer, Near(data, data.tbl_mission_blank));
    Call(ds, event, POSITION, ColumnX(data, window, columns, -0x20), RowY(data, window, m));
    Call(ds, event, TEXT_COLOR, TABLE_TEXT_COLOR);
    Call(ds, event, PUT_RANGE, buffer, columns.start, columns.count);
  }
  Call(ds, event, TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The services of the row-draw callbacks: the column layout of the active list window, the text output and the
// message lookup.
uint16_t svc_table(uint8_t* ds, const char* module, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case COLUMNS:      opeu_table_columns(ds, svc_listui, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case POSITION:     opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case FORMAT:       opeu_request_format(ds, { args, count }); break;
  case TEXT_COLOR:   opeu_text_color(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case SIDE_COLOR:   opeu_select_side_color(ds, svc_sideutil, (uint8_t)Arg(args, count, 0)); break;
  case UI_TEXT:      opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case PRINT:        opeu_cursor_print(ds, { args, count }); break;
  case PUT_RANGE:    opeu_text_put_range(ds, (TextHost*)port_textout_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case REQUEST_TEXT: r = opeu_message_lookup(ds, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  default:           port_unhandled(module, id, count);
  }
  return r;
}

uint16_t svc_tblgroup(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_table(ds, "tblgroup", id, args, count);
}

auto port_routine_opeu_tbl_draw_groups(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(ds, svc_tblgroup, opeu_tbl_draw_groups, args, count);
}

auto port_routine_opeu_tbl_draw_missions(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(ds, svc_tblgroup, opeu_tbl_draw_missions, args, count);
}
