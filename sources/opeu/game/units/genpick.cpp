// The general-list choice and row-draw callbacks of the staff and division assignment screen ("Who will be your
// Chief of ..."). The listed generals are the near pointers in general_list; list_window is the list's window.
#include "units/genpick.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "common/side-color.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "core/divstr.hpp"
#include "core/personfl.hpp"
#include "core/sideutil.hpp"
#include "core/staffpnl.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "core/vacancy.hpp"
#include "io/listui.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

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

// Whether the general holds a staff role or commands a division.
[[nodiscard]] auto HasPost(Person const& general) -> bool {
  return (general.flags & Bits(PersonRole::ROLE_MASK)) || (general.assignment & Bits(Assignment::DIVISION));
}

// Restores the bottom panel and prints a prompt with one argument.
template <typename... _Arguments>
auto Prompt(Game& game, std::uint16_t text, _Arguments... arguments) -> void {
  opeu_staff_restore_bottom(game);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(PROMPT_X), static_cast<int16_t>(PROMPT_Y));
  opeu_cursor_print(game, std::array<std::uint16_t, 1 + sizeof...(_Arguments)>{ text, static_cast<std::uint16_t>(arguments)... });
  opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(0));
}
} // namespace

// Choice callback: 0..2 staff roles, 3..6 division commanders, 11..13 dismiss a staff role, 14.. dismiss a division
// commander. Returns 1 after a change, 0 when the general is already assigned.
uint8_t opeu_general_assign_choice(Game& game, uint8_t choice) {
  auto&               data{ Data(game) };
  int const           staff_kinds[3]{ 1, 2, 3 };
  std::uint16_t const person{ data.general_list[data.list_row] };
  if (choice <= 10 && HasPost(Record<Person>(data, person))) return 0;
  switch (choice) {
  case 0:
    opeu_person_set_role(game, person, static_cast<uint8_t>(1));
    opeu_context_portrait(game, static_cast<const UiHost*>(port_uiprim_host()), 0x20, 0x28, person);
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(HIGHLIGHT));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x29), static_cast<int16_t>(0x58));
    opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(OPS_FORMAT), static_cast<std::uint16_t>(SideTextColor(data.current_side)), static_cast<std::uint16_t>(person) });
    Prompt(game, OPS_PROMPT);
    return 1;
  case 1:
    opeu_person_set_role(game, person, static_cast<uint8_t>(2));
    opeu_context_portrait(game, static_cast<const UiHost*>(port_uiprim_host()), 0xc, 0x28, person);
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(HIGHLIGHT));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x15), static_cast<int16_t>(0x58));
    opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(INTEL_FORMAT), static_cast<std::uint16_t>(SideTextColor(data.current_side)), static_cast<std::uint16_t>(person) });
    Prompt(game, INTEL_PROMPT);
    return 1;
  case 2: {
    opeu_person_set_role(game, person, static_cast<uint8_t>(3));
    opeu_context_portrait(game, static_cast<const UiHost*>(port_uiprim_host()), 0x34, 0x28, person);
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(HIGHLIGHT));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(0x3d), static_cast<int16_t>(0x58));
    opeu_cursor_print(game,
                      std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(DIVISION_FORMAT), static_cast<std::uint16_t>(SideTextColor(data.current_side)), static_cast<std::uint16_t>(person) });
    auto const post{ static_cast<std::uint8_t>(opeu_find_vacant_post(game)) };
    opeu_show_division_strength(game, static_cast<uint8_t>(static_cast<std::uint8_t>(post - 3)));
    Prompt(game, DIVISION_PROMPT, post - 2);
    return 1;
  }
  case 3:
  case 4:
  case 5:
  case 6: {
    opeu_person_set_group(game, person, static_cast<uint8_t>(static_cast<std::uint8_t>(choice - 3)));
    opeu_context_portrait(game, static_cast<const UiHost*>(port_uiprim_host()), static_cast<std::uint16_t>(choice * 20 - 0x3a), 0x88, person);
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(HIGHLIGHT));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(static_cast<std::uint16_t>(choice * 20 - 0x31)), static_cast<int16_t>(0x98));
    opeu_cursor_print(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(choice == 6 ? DIVISION_SHORT : DIVISION_LONG), static_cast<std::uint16_t>(SideTextColor(data.current_side)),
                                                          static_cast<std::uint16_t>(person) });
    auto const post{ static_cast<std::uint8_t>(opeu_find_vacant_post(game)) };
    if (post >= 7) {
      opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(0));
      return 1;
    }
    opeu_show_division_strength(game, static_cast<uint8_t>(static_cast<std::uint8_t>(post - 3)));
    Prompt(game, DIVISION_PROMPT, post - 2);
    return 1;
  }
  default: {
    if (choice <= 10) return 0;
    int const dismissal{ choice - 10 };
    int const slot{ choice - 11 };
    opeu_text_paper_set(game, static_cast<TextHost*>(port_textout_host()), static_cast<uint8_t>(HIGHLIGHT));
    if (dismissal <= 3)
      opeu_clear_staff_role(game, static_cast<uint8_t>(data.current_side), static_cast<uint8_t>(static_cast<std::uint8_t>(staff_kinds[slot])));
    else
      opeu_clear_division_command(game, static_cast<uint8_t>(data.current_side), static_cast<uint8_t>(static_cast<std::uint8_t>(dismissal - 4)));
    opeu_staff_frame_option(game, static_cast<int16_t>(slot));
    opeu_staff_mark_option(game, static_cast<int16_t>(slot));
    if (dismissal <= 3)
      Prompt(game, ROLE_PROMPT, data.role_names[slot]);
    else
      Prompt(game, DIVISION_PROMPT, dismissal - 3);
    return 1;
  }
  }
}

// Row-draw callback for list rows first..last (last below 255). Flags bit 0 or 1 draws the ability header, bit 0 or
// 2 the names.
void opeu_general_list_draw_rows(Game& game, uint8_t column, uint8_t first, uint8_t row, uint8_t last, uint8_t flags) {
  auto&       data{ Data(game) };
  auto const& window{ Record<ListWindow>(data, data.list_window) };
  auto const  row_width{ [&] { return Record<std::uint8_t>(data, window.widths); } };
  auto const  first_column{ static_cast<std::uint8_t>(TableColumnLayout(game, static_cast<std::uint8_t>(column), static_cast<std::uint8_t>(row)).lead) };
  if (flags & 3) {
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(row_width() + first_column + window.x - 0x1f), static_cast<int16_t>(window.y + 1));
    opeu_select_side_color(game, static_cast<uint8_t>(data.current_side));
    opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(HEADER) });
  }
  for (std::uint8_t index{ first }; index <= last; ++index) {
    auto const  person{ data.general_list[index] };
    auto const& general{ Record<Person>(data, person) };
    if (general.status == PERSON_VACANT) continue;
    auto const y{ (index - data.list_top) * ROW_HEIGHT + window.y + 0x12 };
    if (flags & 5) {
      opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(window.x + 3), static_cast<int16_t>(y));
      opeu_select_side_color(game, static_cast<uint8_t>(data.current_side));
      opeu_cursor_print(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(NAME_FORMAT), static_cast<std::uint16_t>(person) });
    }
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), static_cast<int16_t>(row_width() + first_column + window.x - 0x1e), static_cast<int16_t>(y));
    if (HasPost(general))
      opeu_select_side_color(game, static_cast<uint8_t>(data.current_side));
    else
      opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), GREY);
    opeu_cursor_print(game, std::array<std::uint16_t, 8>{ static_cast<std::uint16_t>(STATS_FORMAT), static_cast<std::uint16_t>(general.leadership), static_cast<std::uint16_t>(general.charm),
                                                          static_cast<std::uint16_t>(general.bravery), static_cast<std::uint16_t>(general.health), static_cast<std::uint16_t>(general.infantry),
                                                          static_cast<std::uint16_t>(general.artillery), static_cast<std::uint16_t>(general.armor) });
  }
  opeu_text_color(game, static_cast<TextHost*>(port_textout_host()), GREY);
}

auto port_routine_opeu_general_assign_choice(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_general_assign_choice(game, (uint8_t)Arg(args, count, 0));
}

auto port_routine_opeu_general_list_draw_rows(Game& game, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_general_list_draw_rows(game, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1), (uint8_t)Arg(args, count, 2), (uint8_t)Arg(args, count, 3), (uint8_t)Arg(args, count, 4));
  return 0;
}
