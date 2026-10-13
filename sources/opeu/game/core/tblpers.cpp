// The row-draw callback of the general table: each general's name, nation, abilities, post and whether resting or
// hurt (callback shape and services as core/tblgroup).
#include "core/tblpers.hpp"
#include "core/counter.hpp"
#include "core/personfl.hpp"
#include "core/tblgroup.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "common/table-rows.hpp"
#include "glue.hpp"
#include "port.hpp"
#include "services.hpp"

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
void opeu_tbl_draw_staff(uint8_t ds[65536], HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&               data{ Data(ds) };
  ServiceLocals const locals{ ROW_TEXT_BYTES };
  locals.Clear(ds);
  auto const  buffer{ locals.At(0) };
  auto const& window{ ActiveWindow(data) };
  auto const  columns{ Columns(ds, event, column, row) };
  if (flags & DRAW_HEADER) {
    auto const header{ Call(ds, event, REQUEST_TEXT, HEADER_MESSAGE) };
    Call(ds, event, FORMAT, buffer, header);
    Call(ds, event, POSITION, ColumnX(data, window, columns, -0x1f), window.y + 1);
    Call(ds, event, SIDE_COLOR, data.current_side);
    Call(ds, event, PUT_RANGE, buffer, columns.start, columns.count);
  }
  for (std::uint8_t e = first; e <= last; ++e) {
    auto const  near{ data.general_list[e] };
    auto const& person{ Record<Person>(data, near) };
    if (flags & DRAW_LABELS) {
      Call(ds, event, POSITION, window.x + 3, RowY(data, window, e));
      Call(ds, event, SIDE_COLOR, opeu_person_lacks_side0_flag(ds, near));
      Call(ds, event, PRINT, Near(data, data.tbl_person_name), near);
    }
    auto post{ Near(data, data.tbl_person_blank) };
    auto status{ post };
    if (auto const role{ person.role & Bits(PersonRole::ROLE_MASK) })
      post = data.tbl_post_names[role + STAFF_POSTS_AT];
    else if (person.assignment & Bits(Assignment::DIVISION))
      post = data.tbl_post_names[person.assignment & Bits(Assignment::DIVISION_MASK)];
    if (opeu_get_record_counter(ds + near))
      status = Near(data, data.tbl_person_rest);
    else if (person.role & Bits(PersonRole::POOR_HEALTH))
      status = Near(data, data.tbl_person_hurt);
    auto const format{ Call(ds, event, REQUEST_TEXT, ROW_MESSAGE) };
    auto const nation{ opeu_first_mask_name(ds, person.status) };
    Call(ds, event, FORMAT, buffer, format, nation, person.leadership, person.charm, person.bravery, person.health, person.infantry, person.artillery, person.armor, person.experience, post, status);
    Call(ds, event, POSITION, ColumnX(data, window, columns, -0x1f), RowY(data, window, e));
    Call(ds, event, TEXT_COLOR, TABLE_TEXT_COLOR);
    Call(ds, event, PRINT, buffer, columns.start, columns.count);
  }
  Call(ds, event, TEXT_COLOR, TABLE_TEXT_COLOR);
}

uint16_t svc_tblpers(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_table(ds, "tblpers", id, args, count);
}

auto port_routine_opeu_tbl_draw_staff(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return RunTableRows(ds, svc_tblpers, opeu_tbl_draw_staff, args, count);
}
