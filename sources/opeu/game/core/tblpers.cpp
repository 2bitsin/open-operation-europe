// The row-draw callback of the general table: each general's name, nation, abilities, post and whether resting or
// hurt (callback shape and services as core/tblgroup).
#include "core/tblpers.hpp"
#include "core/counter.hpp"
#include "core/msglook.hpp"
#include "core/personfl.hpp"
#include "core/sideutil.hpp"
#include "core/tblgroup.hpp"
#include "core/textout.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"
#include <array>

using namespace opeu::game;

namespace {

using enum TableService;

constexpr std::uint8_t DRAW_HEADER{ 3 }; // flags that draw the header line
constexpr std::uint8_t DRAW_LABELS{ 5 }; // flags that draw the general names

// Message ids of the table's header line and row format.
constexpr std::uint16_t HEADER_MESSAGE{ 0x7a };
constexpr std::uint16_t ROW_MESSAGE{ 0x78 };

// The row text the format service writes.
constexpr std::uint16_t ROW_TEXT_BYTES{ 0x6c };

// tbl_post_names: the four divisions, then the staff posts by role.
constexpr unsigned STAFF_POSTS_AT{ 3 };

} // namespace

// The general table. A general's post is his staff role when he has one, else his division when assigned to one; the
// status column shows Rest while his counter runs, else Hurt when his health is poor. The formatted row is printed
// as a format string with the visible start and count as its arguments.
void opeu_tbl_draw_staff(Game& game, HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(game) };
  ServiceLocals const locals{ ROW_TEXT_BYTES };
  locals.Clear(game);
  auto const  buffer{ locals.At(0) };
  auto const& window{ ActiveWindow(data) };
  auto const  columns{ TableColumnLayout(game, column, row) };
  if (flags & DRAW_HEADER) {
    auto const header{ opeu_message_lookup(game, static_cast<int16_t>(HEADER_MESSAGE)) };
    opeu_request_format(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(header) });
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(ColumnX(data, window, columns, -0x1f)), static_cast<int16_t>(window.y + 1));
    opeu_select_side_color(game, svc_sideutil, static_cast<uint8_t>(data.current_side));
    opeu_text_put_range(game, static_cast<TextHost*>(port_textout_host()), buffer, static_cast<uint8_t>(columns.start), static_cast<uint8_t>(columns.count));
  }
  for (std::uint8_t e = first; e <= last; ++e) {
    auto const  near{ data.general_list[e] };
    auto const& person{ Record<Person>(data, near) };
    if (flags & DRAW_LABELS) {
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(window.x + 3), static_cast<int16_t>(RowY(data, window, e)));
      opeu_select_side_color(game, svc_sideutil, static_cast<uint8_t>(opeu_person_lacks_side0_flag(game, near)));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(Near(data, data.tbl_person_name)), static_cast<std::uint16_t>(near) });
    }
    auto post{ Near(data, data.tbl_person_blank) };
    auto status{ post };
    if (auto const role{ person.role & Bits(PersonRole::ROLE_MASK) })
      post = data.tbl_post_names[role + STAFF_POSTS_AT];
    else if (person.assignment & Bits(Assignment::DIVISION))
      post = data.tbl_post_names[person.assignment & Bits(Assignment::DIVISION_MASK)];
    if (opeu_get_record_counter(Record<Person>(Data(game), near)))
      status = Near(data, data.tbl_person_rest);
    else if (person.role & Bits(PersonRole::POOR_HEALTH))
      status = Near(data, data.tbl_person_hurt);
    auto const format{ opeu_message_lookup(game, static_cast<int16_t>(ROW_MESSAGE)) };
    auto const nation{ opeu_first_mask_name(game, person.status) };
    opeu_request_format(game, std::array<std::uint16_t, 13>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(format), static_cast<std::uint16_t>(nation),
                                                             static_cast<std::uint16_t>(person.leadership), static_cast<std::uint16_t>(person.charm), static_cast<std::uint16_t>(person.bravery),
                                                             static_cast<std::uint16_t>(person.health), static_cast<std::uint16_t>(person.infantry), static_cast<std::uint16_t>(person.artillery),
                                                             static_cast<std::uint16_t>(person.armor), static_cast<std::uint16_t>(person.experience), static_cast<std::uint16_t>(post),
                                                             static_cast<std::uint16_t>(status) });
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(ColumnX(data, window, columns, -0x1f)), static_cast<int16_t>(RowY(data, window, e)));
    opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
    opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(buffer), static_cast<std::uint16_t>(columns.start), static_cast<std::uint16_t>(columns.count) });
  }
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), TABLE_TEXT_COLOR);
}

uint16_t svc_tblpers(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_table(game, "tblpers", id, args, count);
}

auto port_routine_opeu_tbl_draw_staff(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(game, svc_tblpers, opeu_tbl_draw_staff, args, count);
}
