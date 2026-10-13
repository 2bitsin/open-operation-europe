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
#include <array>

using namespace opeu::game;

namespace {

constexpr std::uint8_t DRAW_HEADER{ 3 }; // flags that draw the header line
constexpr std::uint8_t DRAW_LABELS{ 5 }; // flags that draw the row labels

// The row texts the format service writes: the row (and in the mission table the target's place behind it).
constexpr std::uint16_t ROW_TEXT_BYTES{ 0x6c };
constexpr std::uint16_t PLACE_AT{ 60 };
} // namespace

// The division table: each division's label and, for a division with an eligible selected unit, its commander and
// its fuel, ammunition and food percentages.
void opeu_tbl_draw_groups(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ ROW_TEXT_BYTES };
  locals.Clear(game);
  auto const  buffer{ locals.At(0) };
  auto const& window{ ActiveWindow(data) };
  auto const  columns{ TableColumnLayout(game, column, row) };
  auto const  side{ data.current_side };
  if (flags & DRAW_HEADER) {
    opeu_request_format(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(Near(data, data.tbl_group_header)) });
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(ColumnX(data, window, columns, -0x20)), static_cast<int16_t>(window.y + 1));
    opeu_select_side_color(game, static_cast<uint8_t>(side));
    opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.tbl_group_title)), static_cast<std::uint16_t>(buffer) });
  }
  for (std::uint8_t g = first; g <= last; ++g) {
    auto const& group{ GroupOf(data, side, g) };
    if (flags & DRAW_LABELS) {
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(window.x + 2), static_cast<int16_t>(RowY(data, window, g)));
      opeu_select_side_color(game, static_cast<uint8_t>(side));
      if (group.flags & Bits(GroupFlag::READY)) {
        if (group.flags & Bits(GroupFlag::SELECTED)) opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(Near(data, data.tbl_group_mark)) });
        opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.tbl_group_division)), static_cast<std::uint16_t>(g + 1) });
      } else
        opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.tbl_group_empty));
    }
    auto const group_near{ Near(data, group) };
    if ((group.flags & Bits(GroupFlag::READY)) && opeu_unit_selection_eligible(game, opeu_find_group_selected_unit(game, group_near))) {
      auto const    commander{ opeu_find_group_record(game, side, g) };
      std::uint16_t ammo;
      std::uint16_t fuel;
      std::uint16_t food;
      opeu_group_resource_percentages(game, group_near, &ammo, &fuel, &food);
      auto const kind{ (group.flags & Bits(GroupFlag::AI_MARK)) ? Near(data, data.tbl_group_div) : Near(data, data.tbl_group_army) };
      opeu_request_format(game,
                          std::array<std::uint16_t, 7>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(Near(data, data.tbl_group_row)), static_cast<std::uint16_t>(kind),
                                                        static_cast<std::uint16_t>(commander), static_cast<std::uint16_t>(fuel), static_cast<std::uint16_t>(ammo), static_cast<std::uint16_t>(food) });
    } else
      opeu_string_copy(game, buffer, Near(data, data.tbl_group_blank));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(ColumnX(data, window, columns, -0x20)), static_cast<int16_t>(RowY(data, window, g)));
    opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
    opeu_text_put_range(game, static_cast<TextHost*>(port_textout_host()), buffer, static_cast<uint8_t>(columns.start), static_cast<uint8_t>(columns.count));
  }
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
}

// The mission table of the current division: each of its four tasks shows its order and the objective at the task's
// cell, or the cell in map coordinates.
void opeu_tbl_draw_missions(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ ROW_TEXT_BYTES };
  locals.Clear(game);
  auto const  buffer{ locals.At(0) };
  auto const  place{ locals.At(PLACE_AT) };
  auto const& window{ ActiveWindow(data) };
  auto const  columns{ TableColumnLayout(game, column, row) };
  auto const  side{ data.current_side };
  if (flags & DRAW_HEADER) {
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x26), static_cast<int16_t>(0x81));
    opeu_select_side_color(game, static_cast<uint8_t>(side));
    opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(Near(data, data.tbl_mission_header)) });
  }
  auto const& tasks{ GroupOf(data, side, data.current_group).tasks };
  for (std::uint8_t m = first; m <= last; ++m) {
    auto const& task{ tasks[m] };
    if (flags & DRAW_LABELS) {
      opeu_select_side_color(game, static_cast<uint8_t>(side));
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(window.x + 2), static_cast<int16_t>(RowY(data, window, m)));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.tbl_mission_label)), static_cast<std::uint16_t>(m + 1) });
    }
    if (task.kind != Bits(TaskKind::NONE)) {
      auto const x{ task.x };
      auto const y{ task.y };
      if (auto const objective{ opeu_find_objective_at(game, x, y) })
        opeu_request_format(game,
                            std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(place), static_cast<std::uint16_t>(Near(data, data.tbl_mission_target)), static_cast<std::uint16_t>(objective) });
      else
        opeu_request_format(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(place), static_cast<std::uint16_t>(Near(data, data.tbl_mission_place)),
                                                                static_cast<std::uint16_t>(x * data.map_scale), static_cast<std::uint16_t>(y * data.map_scale) });
      opeu_request_format(game, std::array<std::uint16_t, 4>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(Near(data, data.tbl_mission_row)),
                                                              static_cast<std::uint16_t>(data.order_names[task.kind]), static_cast<std::uint16_t>(place) });
    } else
      opeu_string_copy(game, buffer, Near(data, data.tbl_mission_blank));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(ColumnX(data, window, columns, -0x20)), static_cast<int16_t>(RowY(data, window, m)));
    opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
    opeu_text_put_range(game, static_cast<TextHost*>(port_textout_host()), buffer, static_cast<uint8_t>(columns.start), static_cast<uint8_t>(columns.count));
  }
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
}

auto port_routine_opeu_tbl_draw_groups(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, opeu_tbl_draw_groups, args, count);
}

auto port_routine_opeu_tbl_draw_missions(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, opeu_tbl_draw_missions, args, count);
}
