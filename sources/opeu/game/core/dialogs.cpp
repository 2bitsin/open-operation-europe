// The turn frame, the context, info, title and message panels, the speaker dialogs (a general's portrait and name
// beside the message), the prompts and the unit and objective reports.
//
// Every call that leaves the module goes through the host's event; the scratch buffers live in the DS area at
// host->scratch (at most 132 bytes; no two buffer owners are active at once).
#include "core/dialogs.hpp"
#include "common/events.hpp"
#include "common/host.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "common/side-color.hpp"
#include "core/context.hpp"
#include "core/msglook.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/perssel.hpp"
#include "core/presenters.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "map/objeleg.hpp"
#include "map/query.hpp"
#include "map/support.hpp"
#include "port.hpp"
#include "services.hpp"
#include "units/labels.hpp"
#include "units/stats.hpp"
#include "units/unitname.hpp"
#include <array>

using namespace opeu::game;

namespace {

// Far data: the panel border tiles and the turn frame pictures, as segments relative to the load segment.
constexpr std::uint16_t PANEL_SEGMENT{ 0x3ab9 };

constexpr std::uint8_t  HURRICANE_WEATHER{ 5 }; // storms are named hurricanes in scenario 3 from July to September
constexpr Scenario      HURRICANE_SCENARIO{ Scenario::D_DAY };
constexpr std::uint16_t NEW_WEAPON_PICTURE{ 0xe7 };   // the daily picture that plays the fanfare
constexpr std::uint16_t INTELLIGENCE_CHIEF_KIND{ 3 }; // find_side_record: the general who reports on objectives
constexpr std::uint16_t ONE_HOUR_TEXT{ 0x77 };        // message id of the one-hour delay confirmation

// A far pointer into the far data loaded with the game.
struct Far {
  std::uint16_t segment;
  std::uint16_t offset;
};

auto Loaded(DialogHost const* host, std::uint16_t segment, std::uint16_t offset) -> Far {
  return { static_cast<std::uint16_t>(host->load + segment), offset };
}

} // namespace

// Draws the border of panel `which` (0 context, 1 title, 2 message) from the panel tiles.
void opeu_panel_border(Game& game, const DialogHost* host, uint16_t which) {
  auto const& data{ Data(game) };
  auto const  corner{ Loaded(host, PANEL_SEGMENT, which == 0 ? 0 : 0x100) };
  auto const  edge{ Loaded(host, PANEL_SEGMENT, which == 0 ? 128 : 0x100 + 128) };
  auto const  last_column{ data.panel_columns[which] };
  auto const  row{ data.panel_rows[which] };
  opeu_bitmap_restore(0x2c, row, 1, 0x20, 0, port_far(corner.segment, corner.offset));
  for (std::uint16_t column{ 0x2d }; column <= last_column; ++column) opeu_bitmap_restore(column, row, 1, 0x20, 0, port_far(edge.segment, edge.offset));
}

// Puts up the message backdrop once per screen; nonzero when it was drawn now.
uint16_t opeu_message_backdrop(Game& game, const DialogHost* host) {
  auto& data{ Data(game) };
  if (data.context_display_word == 0) {
    opeu_picture_memory_masked(game, static_cast<const UiHost*>(port_uiprim_host()), 0x160, 0x18, data.frame_segment, 0x1000);
    return data.context_display_word = 1;
  }
  return 0;
}

// The date line of the turn frame: month, day, year, hour and the weather (a hurricane in scenario 3's summer).
void opeu_turn_prepare(Game& game, const DialogHost* host) {
  auto const&   data{ Data(game) };
  std::uint16_t weather;
  if (data.weather == HURRICANE_WEATHER && CurrentScenario(data) == HURRICANE_SCENARIO && data.month >= 7 && data.month <= 9)
    weather = Near(data, data.hurricane_name);
  else
    weather = data.weather_names[data.weather];
  opeu_picture_memory_masked(game, static_cast<const UiHost*>(port_uiprim_host()), 0, 0x18, data.frame_segment, 0x1c00);
  opeu_context_save(game, static_cast<TextHost*>(port_textout_host()));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x12, 0x20);
  opeu_cursor_print(game, std::array<std::uint16_t, 6>{ Near(data, data.turn_date_format), static_cast<std::uint16_t>(data.month), static_cast<std::uint16_t>(data.day),
                                                        static_cast<std::uint16_t>(data.year), static_cast<std::uint16_t>(data.hour), weather });
  opeu_context_restore(game, static_cast<TextHost*>(port_textout_host()));
  opeu_palette_for_hour(game, static_cast<const UiHost*>(port_uiprim_host()));
}

// Draws the turn frame in `mode` (0: the map frame, else the full frame) unless it is up already.
void opeu_turn_frame(Game& game, const DialogHost* host, uint8_t mode) {
  auto& data{ Data(game) };
  if (mode == data.turn_frame_mode) return;
  opeu_turn_prepare(game, host);
  opeu_picture_memory_masked(game, static_cast<const UiHost*>(port_uiprim_host()), 0, 0x38, data.frame_segment, 0x400);
  bool const          map{ mode == 0 };
  std::uint16_t const offset{ CurrentScenario(data) == Scenario::AFRICA ? (map ? std::uint16_t{ 0x120 } : std::uint16_t{ 0x360 }) : (map ? std::uint16_t{ 0x240 } : std::uint16_t{ 0x4e0 }) };
  auto const          frame{ Loaded(host, FRAME_SEGMENT, offset) };
  data.stream_ptr[0] = frame.offset;
  data.stream_ptr[1] = frame.segment;
  opeu_bitmap_restore(0x13, map ? 0x38 : 0x44, 6, map ? 0xc : 0x10, 0, port_far(data.stream_ptr[1], data.stream_ptr[0]));
  data.turn_frame_mode = mode;
}

// The active-turn panel at the top of the screen.
void opeu_turn_active_begin(Game& game, const DialogHost* host) {
  auto& data{ Data(game) };
  if (data.control_selection == 1)
    opeu_ui_box(1, 3, 0x4e, 0x12, 0xf);
  else {
    opeu_picture_memory_masked(game, static_cast<const UiHost*>(port_uiprim_host()), 0, 0, data.frame_segment, 0);
    data.control_selection = 1;
  }
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.turn_panel_text));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 2, 4);
}

void opeu_context_begin(Game& game, const DialogHost* host) {
  auto& data{ Data(game) };
  if (!opeu_message_backdrop(game, host)) {
    opeu_panel_border(game, host, 0);
    opeu_ui_box(0x2d, 0x38, 0x14, 0x42, 1);
  }
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.context_panel_text));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x2d, 0x39);
  data.context_side = NO_SIDE;
}

void opeu_info_panel_begin(Game& game, const DialogHost* host) {
  auto& data{ Data(game) };
  if (!opeu_message_backdrop(game, host))
    opeu_ui_box(0x2d, 0x9a, 0x22, 0x80, 1);
  else
    data.context_side = NO_SIDE;
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.info_panel_text));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x2d, 0xa3);
}

void opeu_title_panel_begin(Game& game, const DialogHost* host) {
  auto& data{ Data(game) };
  if (!opeu_message_backdrop(game, host)) {
    opeu_panel_border(game, host, 1);
    opeu_ui_box(0x41, 0x84, 0xe, 0x16, 1);
    opeu_ui_box(0x2d, 0x9a, 0x22, 0x80, 1);
  } else
    data.context_side = NO_SIDE;
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.title_panel_text));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x2d, 0xa3);
}

void opeu_message_panel_begin(Game& game, const DialogHost* host) {
  auto& data{ Data(game) };
  if (!opeu_message_backdrop(game, host)) {
    opeu_panel_border(game, host, 2);
    opeu_ui_box(0x36, 0x13a, 0x18, 0x52, 1);
  } else
    data.context_side = NO_SIDE;
  opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.message_panel_text));
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x36, 0x144);
}

// Clears the screen and forgets every panel, frame and view drawn on it.
void opeu_screen_reset(Game& game, const DialogHost* host) {
  auto& data{ Data(game) };
  opeu_vga_clear();
  data.control_selection    = 0;
  data.context_display_word = 0;
  data.overview_side        = NO_SIDE;
  data.view_side            = NO_SIDE;
  data.turn_frame_mode      = NO_SIDE;
  data.context_side         = NO_SIDE;
}

// The title panel with `name` framed.
void opeu_army_title(Game& game, const DialogHost* host, uint16_t name) {
  auto const& data{ Data(game) };
  opeu_title_panel_begin(game, host);
  opeu_context_save(game, static_cast<TextHost*>(port_textout_host()));
  opeu_context_frame(game, static_cast<const UiHost*>(port_uiprim_host()), 0x2c, 0x7a, static_cast<std::uint8_t>(opeu_string_length(game, name)) + 3);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.title_format), name });
  opeu_context_restore(game, static_cast<TextHost*>(port_textout_host()));
}

// A daily event for a side the player watches: picture `id` in a framed box (when event pictures are on), then the
// presenter; the new-weapon picture plays its fanfare.
void opeu_daily_show(Game& game, const DialogHost* host, uint16_t id, uint8_t side, Presenter present) {
  auto const& data{ Data(game) };
  if (data.side_status[side] != SIDE_PLAYED) return;
  if (data.message_pictures == 1) {
    opeu_bitmap_save(5, 0x78, 0x22, 0xb8, port_far(data.bitmap_buffer, 0));
    opeu_ui_box(5, 0x78, 0x22, 0xb8, SideTextColor(side));
    opeu_ui_outline(game, static_cast<const UiHost*>(port_uiprim_host()), 0x28, 0x78, 0x137, 0x12f, 7);
    opeu_picture_file(game, static_cast<const UiHost*>(port_uiprim_host()), 0x30, 0x80, data.message_picture_handle, id);
  }
  RunPresenter(game, present);
  if (data.message_pictures == 1 && id == NEW_WEAPON_PICTURE) {
    opeu_delay_units(game, data.message_speed * 7);
    opeu_music_command7(game);
    opeu_sound_cue16(game);
  }
  if (data.message_pictures == 1) opeu_bitmap_restore(5, 0x78, 0x22, 0xb8, 0, port_far(data.bitmap_buffer, 0));
}

// show 1: opens the message panel with the speaker's post, name and portrait and a scrolling text window; show 0:
// closes it. 0 when the speaker's side is not watched or the speaker's unit is hidden from the player.
uint16_t opeu_speaker_panel(Game& game, const DialogHost* host, uint16_t person, uint16_t show) {
  auto const& data{ Data(game) };
  auto const  side{ opeu_person_lacks_side0_flag(game, person) };
  if (data.side_status[side] == SIDE_COMPUTER) return 0;
  auto const unit{ opeu_person_selected_unit(game, person) };
  if (data.side_initializing) opeu_daily_context(game, side);
  if (unit && !opeu_unit_selection_eligible(game, opeu_person_selected_unit(game, person))) return 0;
  if (show == 1) {
    auto const& speaker{ Record<Person>(data, person) };
    opeu_message_panel_begin(game, host);
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.speaker_text));
    if (auto const role{ speaker.role & Bits(PersonRole::ROLE_MASK) }) {
      auto const post{ data.role_names[role - 1] };
      opeu_context_frame(game, static_cast<const UiHost*>(port_uiprim_host()), 0x2c, 0x11a, static_cast<std::uint8_t>(opeu_string_length(game, person) + opeu_string_length(game, post) + 5));
      opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), post);
    } else {
      auto const division{ (speaker.assignment & Bits(Assignment::DIVISION_MASK)) + 1 };
      if (speaker.assignment & Bits(Assignment::DIVISION)) {
        opeu_context_frame(game, static_cast<const UiHost*>(port_uiprim_host()), 0x2c, 0x11a, opeu_string_length(game, person) + 0xe);
        opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.division_title), static_cast<std::uint16_t>(division) });
      } else {
        opeu_context_frame(game, static_cast<const UiHost*>(port_uiprim_host()), 0x2c, 0x11a, opeu_string_length(game, person) + 0xd);
        opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.commander_title), static_cast<std::uint16_t>(division) });
      }
    }
    opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.speaker_name_format), person });
    opeu_context_portrait(game, static_cast<const UiHost*>(port_uiprim_host()), 0x2d, 0x13c, person);
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.speaker_body_text));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x36, 0x144);
    opeu_text_scroll_mode(game, static_cast<TextHost*>(port_textout_host()), 1);
    opeu_text_window(game, static_cast<TextHost*>(port_textout_host()), 0x36, 0x4e, 0x142, 0x192);
  } else {
    opeu_text_scroll_mode(game, static_cast<TextHost*>(port_textout_host()), 0);
    opeu_text_window_reset(game, static_cast<TextHost*>(port_textout_host()));
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), Near(data, data.speaker_end_text));
  }
  return 1;
}

// Puts the zero-terminated text at DS:text out character by character.
void opeu_text_feed(Game& game, const DialogHost* host, uint16_t text) {
  for (char c; (c = Record<char>(Data(game), text)) != 0; ++text) opeu_text_put(game, static_cast<TextHost*>(port_textout_host()), c);
}

// The message spoken by the person, if the player sees the person.
void opeu_control_message(Game& game, const DialogHost* host, uint16_t text, uint16_t person) {
  if (opeu_speaker_panel(game, host, person, 1)) {
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), text);
    opeu_speaker_panel(game, host, person, 0);
  }
}

// The prompt with its range ("(first-last)") and the value chosen.
uint16_t opeu_menu_prompt(Game& game, const DialogHost* host, uint16_t text, uint16_t first, uint16_t last, uint16_t flag, uint16_t menu) {
  auto const& data{ Data(game) };
  auto const  range{ host->scratch };
  opeu_request_format(game, std::array<std::uint16_t, 4>{ range, Near(data, data.menu_prompt_format), first, last });
  opeu_text_feed(game, host, text);
  opeu_text_feed(game, host, range);
  return opeu_choose_value(game, first, last, static_cast<int16_t>(flag), menu);
}

uint16_t opeu_control_choose(Game& game, const DialogHost* host, uint16_t text, uint16_t first, uint16_t last, uint16_t flag, uint16_t person, uint16_t menu) {
  opeu_speaker_panel(game, host, person, 1);
  auto const value{ opeu_menu_prompt(game, host, text, first, last, flag, menu) };
  opeu_speaker_panel(game, host, person, 0);
  return value;
}

uint16_t opeu_delay_dialog(Game& game, const DialogHost* host, uint16_t text, uint16_t last, uint16_t person, uint16_t menu) {
  return opeu_control_choose(game, host, text, 1, last, 1, person, menu);
}

uint16_t opeu_choose_dialog(Game& game, const DialogHost* host, uint16_t text, uint16_t last, uint16_t person, uint16_t menu) {
  return opeu_control_choose(game, host, text, 1, last, 0, person, menu);
}

uint16_t opeu_confirm_prompt(Game& game, const DialogHost* host, uint16_t text) {
  auto const& data{ Data(game) };
  opeu_text_feed(game, host, text);
  opeu_text_feed(game, host, Near(data, data.confirm_space));
  return static_cast<uint16_t>(opeu_yes_no(game));
}

uint16_t opeu_confirm_message(Game& game, const DialogHost* host, uint16_t text, uint16_t person) {
  opeu_speaker_panel(game, host, person, 1);
  auto const answer{ opeu_confirm_prompt(game, host, text) };
  opeu_speaker_panel(game, host, person, 0);
  return answer;
}

uint16_t opeu_confirm_ok(Game& game, const DialogHost* host, uint16_t person) {
  auto const& data{ Data(game) };
  return opeu_confirm_message(game, host, Near(data, data.confirm_ok_text), person);
}

// "Wait n hours?" spoken by the person.
uint16_t opeu_delay_confirm(Game& game, const DialogHost* host, uint16_t person, uint8_t hours) {
  auto const& data{ Data(game) };
  auto const  text{ host->scratch };
  if (hours == 1) return opeu_confirm_message(game, host, opeu_message_lookup(game, ONE_HOUR_TEXT), person);
  opeu_request_format(game, std::array<std::uint16_t, 3>{ text, Near(data, data.delay_hours_format), static_cast<std::uint16_t>(hours) });
  return opeu_confirm_message(game, host, text, person);
}

// A report on the unit to its own side, spoken by its division commander, with the map moved to the unit.
void opeu_unit_report(Game& game, const DialogHost* host, uint16_t unit, uint16_t message) {
  auto const& data{ Data(game) };
  auto const  text{ host->scratch };
  auto const  name{ static_cast<std::uint16_t>(host->scratch + 100) };
  auto const  side{ opeu_unit_side(unit) };
  if (data.side_status[side] == SIDE_PLAYED && opeu_unit_selection_eligible(game, unit)) {
    auto const& record{ Record<Unit>(data, unit) };
    opeu_request_format(
        game, std::array<std::uint16_t, 5>{ text, Near(data, data.unit_report_format), static_cast<std::uint16_t>(SideColorCode(side)), data.side_names[side], opeu_unit_name(game, unit, name) });
    opeu_string_append(game, text, message);
    opeu_arrival_location(game, record.x, record.y, side);
    opeu_control_message(game, host, text, opeu_find_group_record(game, side, opeu_unit_group_index(unit)));
    opeu_arrival_view(game, unit);
  }
}

// A report on the objective to its own side, spoken by the side's intelligence chief, with the objective flashed.
void opeu_objective_report(Game& game, const DialogHost* host, uint16_t objective, uint16_t message) {
  auto const& data{ Data(game) };
  auto const  text{ host->scratch };
  auto const  side{ opeu_objective_side(game, objective) };
  if (data.side_status[side] == SIDE_PLAYED && opeu_objective_eligible(game, objective)) {
    auto const& record{ Record<Objective>(data, objective) };
    opeu_request_format(game, std::array<std::uint16_t, 4>{ text, Near(data, data.objective_report_format), static_cast<std::uint16_t>(SideColorCode(side)), objective });
    opeu_string_append(game, text, message);
    opeu_arrival_location(game, record.x, record.y, side);
    opeu_control_message(game, host, text, opeu_find_side_record(game, side, INTELLIGENCE_CHIEF_KIND));
    opeu_side_cell_flash(game, record.x, record.y, side);
  }
}

// A message about the unit: to the enemy's intelligence chief when the unit is special, else to its own division
// commander when the player sees it.
void opeu_unit_side_report(Game& game, const DialogHost* host, uint16_t unit, uint16_t message) {
  auto const&   data{ Data(game) };
  std::uint16_t speaker;
  auto          side{ (opeu_opposite_side(opeu_unit_side(unit))) };
  if (data.side_status[side] == SIDE_PLAYED && opeu_unit_special(game, unit))
    speaker = opeu_find_side_record(game, side, INTELLIGENCE_CHIEF_KIND);
  else {
    side = opeu_opposite_side(side);
    if (data.side_status[side] == SIDE_PLAYED && opeu_unit_selection_eligible(game, unit))
      speaker = opeu_find_group_record(game, side, opeu_unit_group_index(unit));
    else
      return;
  }
  auto const& record{ Record<Unit>(data, unit) };
  opeu_arrival_location(game, record.x, record.y, side);
  opeu_control_message(game, host, message, speaker);
}

// A message about the objective to the enemy's intelligence chief, else to its own side's when the player sees it.
void opeu_objective_side_report(Game& game, const DialogHost* host, uint16_t objective, uint16_t message) {
  auto const& data{ Data(game) };
  auto        side{ (opeu_opposite_side(opeu_objective_side(game, objective))) };
  if (data.side_status[side] != SIDE_PLAYED) {
    side = opeu_opposite_side(side);
    if (data.side_status[side] != SIDE_PLAYED || !opeu_objective_eligible(game, objective)) return;
  }
  auto const& record{ Record<Objective>(data, objective) };
  opeu_arrival_location(game, record.x, record.y, side);
  opeu_control_message(game, host, message, opeu_find_side_record(game, side, INTELLIGENCE_CHIEF_KIND));
}

// The element type's panel: name, face and ratings (attack soft/hard, defense, speed, move range, gun range).
void opeu_element_type_panel(Game& game, const DialogHost* host, uint16_t type) {
  auto const& data{ Data(game) };
  auto const& record{ Record<ElementType>(data, type) };
  auto const  index{ static_cast<std::int16_t>(static_cast<std::uint16_t>(type - Near(data, data.element_types))) / static_cast<int>(sizeof(ElementType)) };
  opeu_army_title(game, host, record.name);
  opeu_face_draw(game, static_cast<const UiHost*>(port_uiprim_host()), 0x2d, 0xb4, 0, index);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x44, 0xb4);
  opeu_cursor_print(game, std::array<std::uint16_t, 3>{ Near(data, data.attack_format), static_cast<std::uint16_t>(record.anti_personnel), static_cast<std::uint16_t>(record.anti_tank) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x45, 0xc4);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.defense_format), static_cast<std::uint16_t>(record.defense) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x45, 0xd4);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.speed_format), static_cast<std::uint16_t>(record.speed) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x45, 0xe4);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.move_range_format),
                                                        static_cast<std::uint16_t>(!record.category ? 0 : static_cast<std::uint16_t>(record.fuel_economy * record.fuel_capacity) / 10) });
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x45, 0xf4);
  opeu_cursor_print(game, std::array<std::uint16_t, 2>{ Near(data, data.gun_range_format), static_cast<std::uint16_t>(record.gun_range) });
}

// A battalion's panel: its type's name and face, quantity, morale, damage, training and experience.
void opeu_group_element_panel(Game& game, const DialogHost* host, uint16_t element) {
  auto const& data{ Data(game) };
  auto const& record{ Record<Element>(data, element) };
  auto const  name{ host->scratch };
  opeu_army_title(game, host, opeu_type_name(game, Near(data, data.element_types[record.type]), name));
  opeu_face_draw(game, static_cast<const UiHost*>(port_uiprim_host()), 0x32, 0x9a, 0, record.type);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x2e, 0xea);
  opeu_cursor_print(game, std::array<std::uint16_t, 6>{ Near(data, data.group_element_format), record.quantity, static_cast<std::uint16_t>(record.morale), record.damage,
                                                        static_cast<std::uint16_t>(record.training), static_cast<std::uint16_t>(record.experience) });
}

// An equipment item's panel: its type's name and face, quantity and extra word.
void opeu_element_panel(Game& game, const DialogHost* host, uint16_t item) {
  auto const& data{ Data(game) };
  auto const& record{ Record<Item>(data, item) };
  auto const  name{ host->scratch };
  opeu_army_title(game, host, opeu_type_name(game, Near(data, data.element_types[record.type]), name));
  opeu_face_draw(game, static_cast<const UiHost*>(port_uiprim_host()), 0x32, 0x9a, 0, record.type);
  opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x30, 0xea);
  opeu_cursor_print(game, std::array<std::uint16_t, 3>{ Near(data, data.element_format), record.quantity, record.extra });
}
