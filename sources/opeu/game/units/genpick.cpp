// The general-list choice and row-draw callbacks of the staff and division assignment screen ("Who will be your
// Chief of ..."). The listed generals are the near pointers in general_list; list_window is the list's window.
#include "units/genpick.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "types.hpp"

#include "core/divstr.hpp"
#include "core/personfl.hpp"
#include "core/sideutil.hpp"
#include "core/staffpnl.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "core/vacancy.hpp"
#include "glue.hpp"
#include "io/listui.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the callbacks call through their event; pointers are DS offsets.
enum Service : std::uint16_t {
  SET_ROLE               = 0,  // (person, value)
  SET_GROUP              = 1,  // (person, value)
  PORTRAIT               = 2,  // (x, y, person)
  TEXT_ATTRIBUTE         = 3,  // (value)
  TEXT_COLOR             = 4,  // (color)
  POSITION               = 5,  // (x, y)
  PRINT                  = 6,  // (format, arguments...)
  SIDE_COLOR             = 7,  // (side)
  TABLE_COLUMNS          = 8,  // (column, row) -> the first column byte
  RESTORE_BOTTOM         = 9,  // ()
  MARK_OPTION            = 10, // (slot)
  FRAME_OPTION           = 11, // (slot)
  SHOW_DIVISION_STRENGTH = 12, // (division)
  FIND_VACANT_POST       = 13, // () -> post
  CLEAR_STAFF_ROLE       = 14, // (side, kind)
  CLEAR_DIVISION_COMMAND = 15, // (side, group)
};

constexpr std::uint16_t OPS_FORMAT{ offsetof(DataSegment, chief_ops_format) };
constexpr std::uint16_t OPS_PROMPT{ offsetof(DataSegment, chief_ops_prompt) };
constexpr std::uint16_t INTEL_FORMAT{ offsetof(DataSegment, chief_intel_format) };
constexpr std::uint16_t INTEL_PROMPT{ offsetof(DataSegment, chief_intel_prompt) };
constexpr std::uint16_t DIVISION_FORMAT{ offsetof(DataSegment, division_format) };
constexpr std::uint16_t DIVISION_SHORT{ offsetof(DataSegment, division_short_format) };
constexpr std::uint16_t DIVISION_LONG{ offsetof(DataSegment, division_long_format) };
constexpr std::uint16_t ROLE_PROMPT{ offsetof(DataSegment, role_prompt) };
constexpr std::uint16_t DIVISION_PROMPT{ offsetof(DataSegment, division_prompt) };
constexpr std::uint16_t HEADER{ offsetof(DataSegment, general_header) };
constexpr std::uint16_t NAME_FORMAT{ offsetof(DataSegment, general_name_format) };
constexpr std::uint16_t STATS_FORMAT{ offsetof(DataSegment, general_stats_format) };

constexpr std::uint16_t PROMPT_X{ 0x15 };
constexpr std::uint16_t PROMPT_Y{ 0x170 };
constexpr std::uint16_t ROW_HEIGHT{ 17 };
constexpr std::uint16_t HIGHLIGHT{ 4 }; // text attribute of the choice being made
constexpr std::uint16_t GREY{ 7 };      // text colour of a general without a post

// The nation's colour index for the side's name in the prompts.
[[nodiscard]] auto NationColor(DataSegment const& data) -> std::uint16_t {
  return data.current_side == 0 ? 6 : 5;
}

// Whether the general holds a staff role or commands a division.
[[nodiscard]] auto HasPost(Person const& general) -> bool {
  return (general.flags & Bits(PersonRole::ROLE_MASK)) || (general.assignment & Bits(Assignment::DIVISION));
}

// Restores the bottom panel and prints a prompt with one argument.
auto Prompt(std::uint8_t* ds, HostEvent event, std::uint16_t text, auto... arguments) -> void {
  Call(ds, event, RESTORE_BOTTOM);
  Call(ds, event, POSITION, PROMPT_X, PROMPT_Y);
  Call(ds, event, PRINT, text, arguments...);
  Call(ds, event, TEXT_ATTRIBUTE, 0);
}
} // namespace

// Choice callback: 0..2 staff roles, 3..6 division commanders, 11..13 dismiss a staff role, 14.. dismiss a division
// commander. Returns 1 after a change, 0 when the general is already assigned.
uint8_t opeu_general_assign_choice(uint8_t ds[65536], HostEvent event, uint8_t choice) {
  auto&               data{ Data(ds) };
  int const           staff_kinds[3]{ 1, 2, 3 };
  std::uint16_t const person{ data.general_list[data.list_row] };
  if (choice <= 10 && HasPost(Record<Person>(data, person))) return 0;
  switch (choice) {
  case 0:
    Call(ds, event, SET_ROLE, person, 1);
    Call(ds, event, PORTRAIT, 0x20, 0x28, person);
    Call(ds, event, TEXT_ATTRIBUTE, HIGHLIGHT);
    Call(ds, event, POSITION, 0x29, 0x58);
    Call(ds, event, PRINT, OPS_FORMAT, NationColor(data), person);
    Prompt(ds, event, OPS_PROMPT);
    return 1;
  case 1:
    Call(ds, event, SET_ROLE, person, 2);
    Call(ds, event, PORTRAIT, 0xc, 0x28, person);
    Call(ds, event, TEXT_ATTRIBUTE, HIGHLIGHT);
    Call(ds, event, POSITION, 0x15, 0x58);
    Call(ds, event, PRINT, INTEL_FORMAT, NationColor(data), person);
    Prompt(ds, event, INTEL_PROMPT);
    return 1;
  case 2: {
    Call(ds, event, SET_ROLE, person, 3);
    Call(ds, event, PORTRAIT, 0x34, 0x28, person);
    Call(ds, event, TEXT_ATTRIBUTE, HIGHLIGHT);
    Call(ds, event, POSITION, 0x3d, 0x58);
    Call(ds, event, PRINT, DIVISION_FORMAT, NationColor(data), person);
    auto const post{ static_cast<std::uint8_t>(Call(ds, event, FIND_VACANT_POST)) };
    Call(ds, event, SHOW_DIVISION_STRENGTH, static_cast<std::uint8_t>(post - 3));
    Prompt(ds, event, DIVISION_PROMPT, post - 2);
    return 1;
  }
  case 3:
  case 4:
  case 5:
  case 6: {
    Call(ds, event, SET_GROUP, person, static_cast<std::uint8_t>(choice - 3));
    Call(ds, event, PORTRAIT, static_cast<std::uint16_t>(choice * 20 - 0x3a), 0x88, person);
    Call(ds, event, TEXT_ATTRIBUTE, HIGHLIGHT);
    Call(ds, event, POSITION, static_cast<std::uint16_t>(choice * 20 - 0x31), 0x98);
    Call(ds, event, PRINT, choice == 6 ? DIVISION_SHORT : DIVISION_LONG, NationColor(data), person);
    auto const post{ static_cast<std::uint8_t>(Call(ds, event, FIND_VACANT_POST)) };
    if (post >= 7) {
      Call(ds, event, TEXT_ATTRIBUTE, 0);
      return 1;
    }
    Call(ds, event, SHOW_DIVISION_STRENGTH, static_cast<std::uint8_t>(post - 3));
    Prompt(ds, event, DIVISION_PROMPT, post - 2);
    return 1;
  }
  default: {
    if (choice <= 10) return 0;
    int const dismissal{ choice - 10 };
    int const slot{ choice - 11 };
    Call(ds, event, TEXT_ATTRIBUTE, HIGHLIGHT);
    if (dismissal <= 3)
      Call(ds, event, CLEAR_STAFF_ROLE, data.current_side, static_cast<std::uint8_t>(staff_kinds[slot]));
    else
      Call(ds, event, CLEAR_DIVISION_COMMAND, data.current_side, static_cast<std::uint8_t>(dismissal - 4));
    Call(ds, event, FRAME_OPTION, slot);
    Call(ds, event, MARK_OPTION, slot);
    if (dismissal <= 3)
      Prompt(ds, event, ROLE_PROMPT, data.role_names[slot]);
    else
      Prompt(ds, event, DIVISION_PROMPT, dismissal - 3);
    return 1;
  }
  }
}

// Row-draw callback for list rows first..last (last below 255). Flags bit 0 or 1 draws the ability header, bit 0 or
// 2 the names.
void opeu_general_list_draw_rows(uint8_t ds[65536], HostEvent event, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&       data{ Data(ds) };
  auto const& window{ Record<ListWindow>(data, data.list_window) };
  auto const  row_width{ [&] { return Record<std::uint8_t>(data, window.widths); } };
  auto const  first_column{ static_cast<std::uint8_t>(Call(ds, event, TABLE_COLUMNS, column, row)) };
  if (flags & 3) {
    Call(ds, event, POSITION, row_width() + first_column + window.x - 0x1f, window.y + 1);
    Call(ds, event, SIDE_COLOR, data.current_side);
    Call(ds, event, PRINT, HEADER);
  }
  for (std::uint8_t index{ first }; index <= last; ++index) {
    auto const  person{ data.general_list[index] };
    auto const& general{ Record<Person>(data, person) };
    if (general.status == PERSON_VACANT) continue;
    auto const y{ (index - data.list_top) * ROW_HEIGHT + window.y + 0x12 };
    if (flags & 5) {
      Call(ds, event, POSITION, window.x + 3, y);
      Call(ds, event, SIDE_COLOR, data.current_side);
      Call(ds, event, PRINT, NAME_FORMAT, person);
    }
    Call(ds, event, POSITION, row_width() + first_column + window.x - 0x1e, y);
    if (HasPost(general))
      Call(ds, event, SIDE_COLOR, data.current_side);
    else
      Call(ds, event, TEXT_COLOR, GREY);
    Call(ds, event, PRINT, STATS_FORMAT, general.leadership, general.charm, general.bravery, general.health, general.infantry, general.artillery, general.armor);
  }
  Call(ds, event, TEXT_COLOR, GREY);
}

// The services of the callbacks; TABLE_COLUMNS answers the column's lead.
uint16_t svc_genpick(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0;
  switch (id) {
  case SET_ROLE:               opeu_person_set_role(ds, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case SET_GROUP:              opeu_person_set_group(ds, Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case PORTRAIT:               opeu_context_portrait(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case TEXT_ATTRIBUTE:         opeu_text_paper_set(ds, (TextHost*)port_textout_host(), (uint8_t)Arg(args, count, 0)); break;
  case TEXT_COLOR:             opeu_text_color(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case POSITION:               opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case PRINT:                  opeu_cursor_print(ds, { args, count }); break;
  case SIDE_COLOR:             opeu_select_side_color(ds, svc_sideutil, (uint8_t)Arg(args, count, 0)); break;
  case TABLE_COLUMNS:          r = TableColumnLayout(ds, static_cast<std::uint8_t>(Arg(args, count, 0)), static_cast<std::uint8_t>(Arg(args, count, 1))).lead; break;
  case RESTORE_BOTTOM:         opeu_staff_restore_bottom(ds, svc_staffpnl); break;
  case MARK_OPTION:            opeu_staff_mark_option(ds, svc_staffpnl, (int16_t)Arg(args, count, 0)); break;
  case FRAME_OPTION:           opeu_staff_frame_option(ds, svc_staffpnl, (int16_t)Arg(args, count, 0)); break;
  case SHOW_DIVISION_STRENGTH: opeu_show_division_strength(ds, svc_divstr, (uint8_t)Arg(args, count, 0)); break;
  case FIND_VACANT_POST:       r = opeu_find_vacant_post(ds); break;
  case CLEAR_STAFF_ROLE:       opeu_clear_staff_role(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  case CLEAR_DIVISION_COMMAND: opeu_clear_division_command(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1)); break;
  default:                     port_unhandled("genpick", id, count);
  }
  return r;
}

auto port_routine_opeu_general_assign_choice(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_general_assign_choice(ds, svc_genpick, (uint8_t)Arg(args, count, 0));
}

auto port_routine_opeu_general_list_draw_rows(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_general_list_draw_rows(ds, svc_genpick, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2), (uint8_t)Arg(args, count, 3), (uint8_t)Arg(args, count, 4));
  return 0;
}
