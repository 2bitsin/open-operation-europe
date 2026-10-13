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
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "core/msglook.hpp"
#include "core/sideutil.hpp"
#include "core/textout.hpp"
#include "io/listui.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

using enum TableService;

constexpr std::uint8_t DRAW_HEADER{ 3 }; // flags that draw the header line
constexpr std::uint8_t DRAW_LABELS{ 5 }; // flags that draw the row labels

// The row texts the format service writes: the row (and in the mission table the target's place behind it).
constexpr std::uint16_t ROW_TEXT_BYTES{ 0x6c };
constexpr std::uint16_t PLACE_AT{ 60 };
} // namespace

// The division table: each division's label and, for a division with an eligible selected unit, its commander and
// its fuel, ammunition and food percentages.
void opeu_tbl_draw_groups(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ ROW_TEXT_BYTES };
  locals.Clear(game);
  auto const  buffer{ locals.At(0) };
  auto const& window{ ActiveWindow(data) };
  auto const  columns{ Columns(game, event, column, row) };
  auto const  side{ data.current_side };
  if (flags & DRAW_HEADER) {
    Call(game, event, FORMAT, buffer, Near(data, data.tbl_group_header));
    Call(game, event, POSITION, ColumnX(data, window, columns, -0x20), window.y + 1);
    Call(game, event, SIDE_COLOR, side);
    Call(game, event, PRINT, Near(data, data.tbl_group_title), buffer);
  }
  for (std::uint8_t g = first; g <= last; ++g) {
    auto const& group{ GroupOf(data, side, g) };
    if (flags & DRAW_LABELS) {
      Call(game, event, POSITION, window.x + 2, RowY(data, window, g));
      Call(game, event, SIDE_COLOR, side);
      if (group.flags & Bits(GroupFlag::READY)) {
        if (group.flags & Bits(GroupFlag::SELECTED)) Call(game, event, PRINT, Near(data, data.tbl_group_mark));
        Call(game, event, PRINT, Near(data, data.tbl_group_division), g + 1);
      } else
        Call(game, event, UI_TEXT, Near(data, data.tbl_group_empty));
    }
    auto const group_near{ Near(data, group) };
    if ((group.flags & Bits(GroupFlag::READY)) && opeu_unit_selection_eligible(game, opeu_find_group_selected_unit(game, group_near))) {
      auto const    commander{ opeu_find_group_record(game, side, g) };
      std::uint16_t ammo;
      std::uint16_t fuel;
      std::uint16_t food;
      opeu_group_resource_percentages(game, group_near, &ammo, &fuel, &food);
      auto const kind{ (group.flags & Bits(GroupFlag::AI_MARK)) ? Near(data, data.tbl_group_div) : Near(data, data.tbl_group_army) };
      Call(game, event, FORMAT, buffer, Near(data, data.tbl_group_row), kind, commander, fuel, ammo, food);
    } else
      opeu_string_copy(game, buffer, Near(data, data.tbl_group_blank));
    Call(game, event, POSITION, ColumnX(data, window, columns, -0x20), RowY(data, window, g));
    Call(game, event, TEXT_COLOR, TABLE_TEXT_COLOR);
    Call(game, event, PUT_RANGE, buffer, columns.start, columns.count);
  }
  Call(game, event, TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The mission table of the current division: each of its four tasks shows its order and the objective at the task's
// cell, or the cell in map coordinates.
void opeu_tbl_draw_missions(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ ROW_TEXT_BYTES };
  locals.Clear(game);
  auto const  buffer{ locals.At(0) };
  auto const  place{ locals.At(PLACE_AT) };
  auto const& window{ ActiveWindow(data) };
  auto const  columns{ Columns(game, event, column, row) };
  auto const  side{ data.current_side };
  if (flags & DRAW_HEADER) {
    Call(game, event, POSITION, 0x26, 0x81);
    Call(game, event, SIDE_COLOR, side);
    Call(game, event, PRINT, Near(data, data.tbl_mission_header));
  }
  auto const& tasks{ GroupOf(data, side, data.current_group).tasks };
  for (std::uint8_t m = first; m <= last; ++m) {
    auto const& task{ tasks[m] };
    if (flags & DRAW_LABELS) {
      Call(game, event, SIDE_COLOR, side);
      Call(game, event, POSITION, window.x + 2, RowY(data, window, m));
      Call(game, event, PRINT, Near(data, data.tbl_mission_label), m + 1);
    }
    if (task.kind != Bits(TaskKind::NONE)) {
      auto const x{ task.x };
      auto const y{ task.y };
      if (auto const objective{ opeu_find_objective_at(game, x, y) })
        Call(game, event, FORMAT, place, Near(data, data.tbl_mission_target), objective);
      else
        Call(game, event, FORMAT, place, Near(data, data.tbl_mission_place), x * data.map_scale, y * data.map_scale);
      Call(game, event, FORMAT, buffer, Near(data, data.tbl_mission_row), data.order_names[task.kind], place);
    } else
      opeu_string_copy(game, buffer, Near(data, data.tbl_mission_blank));
    Call(game, event, POSITION, ColumnX(data, window, columns, -0x20), RowY(data, window, m));
    Call(game, event, TEXT_COLOR, TABLE_TEXT_COLOR);
    Call(game, event, PUT_RANGE, buffer, columns.start, columns.count);
  }
  Call(game, event, TEXT_COLOR, TABLE_TEXT_COLOR);
}

// The services of the row-draw callbacks: the column layout of the active list window, the text output and the
// message lookup.
uint16_t svc_table(Game& game, const char* module, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case COLUMNS:      opeu_table_columns(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4)); break;
  case POSITION:     opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case FORMAT:       opeu_request_format(game, { args, count }); break;
  case TEXT_COLOR:   opeu_text_color(game, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case SIDE_COLOR:   opeu_select_side_color(game, svc_sideutil, (uint8_t)Arg(args, count, 0)); break;
  case UI_TEXT:      opeu_ui_text(game, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case PRINT:        opeu_cursor_print(game, { args, count }); break;
  case PUT_RANGE:    opeu_text_put_range(game, (TextHost*)port_textout_host(), Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2)); break;
  case REQUEST_TEXT: r = opeu_message_lookup(game, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  default:           port_unhandled(module, id, count);
  }
  return r;
}

uint16_t svc_tblgroup(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_table(game, "tblgroup", id, args, count);
}

auto port_routine_opeu_tbl_draw_groups(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, svc_tblgroup, opeu_tbl_draw_groups, args, count);
}

auto port_routine_opeu_tbl_draw_missions(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, svc_tblgroup, opeu_tbl_draw_missions, args, count);
}
