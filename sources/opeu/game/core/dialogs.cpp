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
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/context.hpp"
#include "core/emsmap.hpp"
#include "core/msglook.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/perssel.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "core/strings.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "glue.hpp"
#include "io/keyboard.hpp"
#include "io/sound.hpp"
#include "io/soundcue.hpp"
#include "io/waitkey.hpp"
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

using namespace opeu::game;

namespace {

// The services the dialogs reach through the host's event. Byte arguments are passed zero-extended; byte results
// use the low byte.
enum Service : std::uint16_t {
  BITMAP_RESTORE     = 0,  // (x, y, columns, rows, mode, segment, offset)
  BITMAP_SAVE        = 1,  // (x, y, columns, rows, segment, offset)
  UI_BOX             = 2,  // (x, y, width, height, color)
  VGA_CLEAR          = 3,  // ()
  DISPLAY_PREPARE    = 4,  // ()
  PICTURE_MEMORY     = 5,  // (x, y, segment, offset)
  PALETTE_FOR_HOUR   = 6,  // ()
  PICTURE_FILE       = 7,  // (x, y, handle, block)
  UI_OUTLINE         = 8,  // (x1, y1, x2, y2, color)
  CONTEXT_FRAME      = 9,  // (x, y, width)
  CONTEXT_PORTRAIT   = 10, // (x, y, person)
  FACE_DRAW          = 11, // (x, y, mode, key)
  CONTEXT_SAVE       = 12, // ()
  CONTEXT_RESTORE    = 13, // ()
  ARMY_POSITION      = 14, // (x, y)
  UI_TEXT            = 15, // (text)
  ARMY_PRINT         = 16, // (format, words...)
  REQUEST_FORMAT     = 17, // (destination, format, words...)
  TEXT_SCROLL_MODE   = 18, // (mode)
  TEXT_WINDOW        = 19, // (x1, x2, y1, y2)
  TEXT_WINDOW_RESET  = 20, // ()
  TEXT_PUTC          = 21, // (character)
  MENU_SELECT        = 22, // (first, last, flag, menu) -> the value chosen
  CONFIRM_INPUT      = 23, // () -> nonzero for yes
  WAIT_TICKS         = 24, // (ticks)
  MUSIC_COMMAND7     = 25, // ()
  SOUND_CUE16        = 26, // ()
  LACKS_SIDE0        = 27, // (person) -> the person's side
  SELECTED_UNIT      = 28, // (person) -> the unit the person commands, 0 for none
  SELECTION_ELIGIBLE = 29, // (unit) -> nonzero when the player may see it
  DAILY_CONTEXT      = 30, // (side)
  STRING_LENGTH      = 31, // (text)
  STRING_APPEND      = 32, // (destination, source)
  REQUEST_TEXT       = 33, // (message id) -> near pointer to the text
  UNIT_SIDE          = 34, // (unit)
  UNIT_NAME          = 35, // (unit, buffer) -> buffer
  ARRIVAL_LOCATION   = 36, // (x, y, side)
  GROUP_INDEX        = 37, // (unit)
  FIND_GROUP_RECORD  = 38, // (side, group) -> the division commander
  ARRIVAL_VIEW       = 39, // (unit)
  OBJECTIVE_SIDE     = 40, // (objective)
  OBJECTIVE_ELIGIBLE = 41, // (objective) -> nonzero when the player may see it
  FIND_SIDE_RECORD   = 42, // (side, kind) -> the side's staff general of that kind
  ARRIVAL_FLASH      = 43, // (x, y, side)
  OPPOSITE_SIDE      = 44, // (side)
  UNIT_SPECIAL       = 45, // (unit)
  TYPE_NAME_TEXT     = 46, // (type record, buffer) -> buffer
  PRESENT            = 47, // (offset, segment) of a far presenter routine
};

// Far data: the panel border tiles and the turn frame pictures, as segments relative to the load segment.
constexpr std::uint16_t PANEL_SEGMENT{ 0x3ab9 };

constexpr std::uint8_t  HURRICANE_WEATHER{ 5 }; // storms are named hurricanes in scenario 3 from July to September
constexpr Scenario      HURRICANE_SCENARIO{ Scenario::D_DAY };
constexpr std::uint16_t NEW_WEAPON_PICTURE{ 0xe7 }; // the daily picture that plays the fanfare
constexpr std::uint8_t  NO_SIDE{ 0xff };
constexpr std::uint16_t INTELLIGENCE_CHIEF_KIND{ 3 }; // find_side_record: the general who reports on objectives
constexpr std::uint16_t ONE_HOUR_TEXT{ 0x77 }; // message id of the one-hour delay confirmation

template <typename... Words>
auto Send(std::uint8_t* ds, DialogHost const* host, Service service, Words... words) -> std::uint16_t {
  return Call(ds, host->event, service, words...);
}

// A far pointer into the far data loaded with the game.
struct Far {
  std::uint16_t segment;
  std::uint16_t offset;
};

auto Loaded(DialogHost const* host, std::uint16_t segment, std::uint16_t offset) -> Far {
  return { static_cast<std::uint16_t>(host->load + segment), offset };
}

// Side letter in the report format: '6' for side 0, '5' for side 1.
constexpr auto SideMark(std::uint8_t side) -> std::uint16_t {
  return side ? '5' : '6';
}
} // namespace

// Draws the border of panel `which` (0 context, 1 title, 2 message) from the panel tiles.
void opeu_panel_border(uint8_t ds[65536], const DialogHost* host, uint16_t which) {
  auto const& data{ Data(ds) };
  auto const  corner{ Loaded(host, PANEL_SEGMENT, which == 0 ? 0 : 0x100) };
  auto const  edge{ Loaded(host, PANEL_SEGMENT, which == 0 ? 128 : 0x100 + 128) };
  auto const  last_column{ data.panel_columns[which] };
  auto const  row{ data.panel_rows[which] };
  Send(ds, host, BITMAP_RESTORE, 0x2c, row, 1, 0x20, 0, corner.segment, corner.offset);
  for (std::uint16_t column{ 0x2d }; column <= last_column; ++column)
    Send(ds, host, BITMAP_RESTORE, column, row, 1, 0x20, 0, edge.segment, edge.offset);
}

// Puts up the message backdrop once per screen; nonzero when it was drawn now.
uint16_t opeu_message_backdrop(uint8_t ds[65536], const DialogHost* host) {
  auto& data{ Data(ds) };
  if (data.context_display_word == 0) {
    Send(ds, host, DISPLAY_PREPARE);
    Send(ds, host, PICTURE_MEMORY, 0x160, 0x18, data.frame_segment, 0x1000);
    return data.context_display_word = 1;
  }
  return 0;
}

// The date line of the turn frame: month, day, year, hour and the weather (a hurricane in scenario 3's summer).
void opeu_turn_prepare(uint8_t ds[65536], const DialogHost* host) {
  auto const&   data{ Data(ds) };
  std::uint16_t weather;
  if (data.weather == HURRICANE_WEATHER && CurrentScenario(data) == HURRICANE_SCENARIO && data.month >= 7
      && data.month <= 9)
    weather = Near(data, data.hurricane_name);
  else
    weather = data.weather_names[data.weather];
  Send(ds, host, DISPLAY_PREPARE);
  Send(ds, host, PICTURE_MEMORY, 0, 0x18, data.frame_segment, 0x1c00);
  Send(ds, host, CONTEXT_SAVE);
  Send(ds, host, ARMY_POSITION, 0x12, 0x20);
  Send(ds, host, ARMY_PRINT, Near(data, data.turn_date_format), data.month, data.day, data.year, data.hour, weather);
  Send(ds, host, CONTEXT_RESTORE);
  Send(ds, host, PALETTE_FOR_HOUR);
}

// Draws the turn frame in `mode` (0: the map frame, else the full frame) unless it is up already.
void opeu_turn_frame(uint8_t ds[65536], const DialogHost* host, uint8_t mode) {
  auto& data{ Data(ds) };
  if (mode == data.turn_frame_mode) return;
  opeu_turn_prepare(ds, host);
  if (data.turn_frame_mode != 1 || mode != 0) Send(ds, host, DISPLAY_PREPARE);
  Send(ds, host, PICTURE_MEMORY, 0, 0x38, data.frame_segment, 0x400);
  bool const          map{ mode == 0 };
  std::uint16_t const offset{ CurrentScenario(data) == Scenario::AFRICA
                                  ? (map ? std::uint16_t{ 0x120 } : std::uint16_t{ 0x360 })
                                  : (map ? std::uint16_t{ 0x240 } : std::uint16_t{ 0x4e0 }) };
  auto const          frame{ Loaded(host, FRAME_SEGMENT, offset) };
  data.stream_ptr[0] = frame.offset;
  data.stream_ptr[1] = frame.segment;
  Send(ds, host, BITMAP_RESTORE, 0x13, map ? 0x38 : 0x44, 6, map ? 0xc : 0x10, 0, data.stream_ptr[1],
       data.stream_ptr[0]);
  data.turn_frame_mode = mode;
}

// The active-turn panel at the top of the screen.
void opeu_turn_active_begin(uint8_t ds[65536], const DialogHost* host) {
  auto& data{ Data(ds) };
  if (data.control_selection == 1)
    Send(ds, host, UI_BOX, 1, 3, 0x4e, 0x12, 0xf);
  else {
    Send(ds, host, DISPLAY_PREPARE);
    Send(ds, host, PICTURE_MEMORY, 0, 0, data.frame_segment, 0);
    data.control_selection = 1;
  }
  Send(ds, host, UI_TEXT, Near(data, data.turn_panel_text));
  Send(ds, host, ARMY_POSITION, 2, 4);
}

void opeu_context_begin(uint8_t ds[65536], const DialogHost* host) {
  auto& data{ Data(ds) };
  if (!opeu_message_backdrop(ds, host)) {
    opeu_panel_border(ds, host, 0);
    Send(ds, host, UI_BOX, 0x2d, 0x38, 0x14, 0x42, 1);
  }
  Send(ds, host, UI_TEXT, Near(data, data.context_panel_text));
  Send(ds, host, ARMY_POSITION, 0x2d, 0x39);
  data.context_side = NO_SIDE;
}

void opeu_info_panel_begin(uint8_t ds[65536], const DialogHost* host) {
  auto& data{ Data(ds) };
  if (!opeu_message_backdrop(ds, host))
    Send(ds, host, UI_BOX, 0x2d, 0x9a, 0x22, 0x80, 1);
  else
    data.context_side = NO_SIDE;
  Send(ds, host, UI_TEXT, Near(data, data.info_panel_text));
  Send(ds, host, ARMY_POSITION, 0x2d, 0xa3);
}

void opeu_title_panel_begin(uint8_t ds[65536], const DialogHost* host) {
  auto& data{ Data(ds) };
  if (!opeu_message_backdrop(ds, host)) {
    opeu_panel_border(ds, host, 1);
    Send(ds, host, UI_BOX, 0x41, 0x84, 0xe, 0x16, 1);
    Send(ds, host, UI_BOX, 0x2d, 0x9a, 0x22, 0x80, 1);
  } else
    data.context_side = NO_SIDE;
  Send(ds, host, UI_TEXT, Near(data, data.title_panel_text));
  Send(ds, host, ARMY_POSITION, 0x2d, 0xa3);
}

void opeu_message_panel_begin(uint8_t ds[65536], const DialogHost* host) {
  auto& data{ Data(ds) };
  if (!opeu_message_backdrop(ds, host)) {
    opeu_panel_border(ds, host, 2);
    Send(ds, host, UI_BOX, 0x36, 0x13a, 0x18, 0x52, 1);
  } else
    data.context_side = NO_SIDE;
  Send(ds, host, UI_TEXT, Near(data, data.message_panel_text));
  Send(ds, host, ARMY_POSITION, 0x36, 0x144);
}

// Clears the screen and forgets every panel, frame and view drawn on it.
void opeu_screen_reset(uint8_t ds[65536], const DialogHost* host) {
  auto& data{ Data(ds) };
  Send(ds, host, VGA_CLEAR);
  data.control_selection    = 0;
  data.context_display_word = 0;
  data.overview_side        = NO_SIDE;
  data.view_side            = NO_SIDE;
  data.turn_frame_mode      = NO_SIDE;
  data.context_side         = NO_SIDE;
}

// The title panel with `name` framed.
void opeu_army_title(uint8_t ds[65536], const DialogHost* host, uint16_t name) {
  auto const& data{ Data(ds) };
  opeu_title_panel_begin(ds, host);
  Send(ds, host, CONTEXT_SAVE);
  Send(ds, host, CONTEXT_FRAME, 0x2c, 0x7a, static_cast<std::uint8_t>(Send(ds, host, STRING_LENGTH, name)) + 3);
  Send(ds, host, ARMY_PRINT, Near(data, data.title_format), name);
  Send(ds, host, CONTEXT_RESTORE);
}

// A daily event for a side the player watches: picture `id` in a framed box (when event pictures are on), then the
// far presenter routine; the new-weapon picture plays its fanfare.
void opeu_daily_show(uint8_t ds[65536], const DialogHost* host, uint16_t id, uint8_t side, uint32_t present) {
  auto const& data{ Data(ds) };
  if (data.side_status[side] != SIDE_PLAYED) return;
  if (data.message_pictures == 1) {
    Send(ds, host, BITMAP_SAVE, 5, 0x78, 0x22, 0xb8, data.bitmap_buffer, 0);
    Send(ds, host, UI_BOX, 5, 0x78, 0x22, 0xb8, side ? 5 : 6);
    Send(ds, host, UI_OUTLINE, 0x28, 0x78, 0x137, 0x12f, 7);
    Send(ds, host, PICTURE_FILE, 0x30, 0x80, data.message_picture_handle, id);
  }
  if (present) Send(ds, host, PRESENT, static_cast<std::uint16_t>(present), static_cast<std::uint16_t>(present >> 16));
  if (data.message_pictures == 1 && id == NEW_WEAPON_PICTURE) {
    Send(ds, host, WAIT_TICKS, static_cast<std::uint8_t>(data.message_speed * 7));
    Send(ds, host, MUSIC_COMMAND7);
    Send(ds, host, SOUND_CUE16);
  }
  if (data.message_pictures == 1) Send(ds, host, BITMAP_RESTORE, 5, 0x78, 0x22, 0xb8, 0, data.bitmap_buffer, 0);
}

// show 1: opens the message panel with the speaker's post, name and portrait and a scrolling text window; show 0:
// closes it. 0 when the speaker's side is not watched or the speaker's unit is hidden from the player.
uint16_t opeu_speaker_panel(uint8_t ds[65536], const DialogHost* host, uint16_t person, uint16_t show) {
  auto const& data{ Data(ds) };
  auto const  side{ static_cast<std::uint8_t>(Send(ds, host, LACKS_SIDE0, person)) };
  if (data.side_status[side] == 0) return 0;
  auto const unit{ Send(ds, host, SELECTED_UNIT, person) };
  if (data.side_initializing) Send(ds, host, DAILY_CONTEXT, side);
  if (unit && !Send(ds, host, SELECTION_ELIGIBLE, Send(ds, host, SELECTED_UNIT, person))) return 0;
  if (show == 1) {
    auto const& speaker{ Record<Person>(data, person) };
    opeu_message_panel_begin(ds, host);
    Send(ds, host, UI_TEXT, Near(data, data.speaker_text));
    if (auto const role{ speaker.role & Bits(PersonRole::ROLE_MASK) }) {
      auto const post{ data.role_names[role - 1] };
      Send(ds, host, CONTEXT_FRAME, 0x2c, 0x11a,
           static_cast<std::uint8_t>(Send(ds, host, STRING_LENGTH, person) + Send(ds, host, STRING_LENGTH, post) + 5));
      Send(ds, host, UI_TEXT, post);
    } else {
      auto const division{ (speaker.assignment & Bits(Assignment::DIVISION_MASK)) + 1 };
      if (speaker.assignment & Bits(Assignment::DIVISION)) {
        Send(ds, host, CONTEXT_FRAME, 0x2c, 0x11a, Send(ds, host, STRING_LENGTH, person) + 0xe);
        Send(ds, host, ARMY_PRINT, Near(data, data.division_title), division);
      } else {
        Send(ds, host, CONTEXT_FRAME, 0x2c, 0x11a, Send(ds, host, STRING_LENGTH, person) + 0xd);
        Send(ds, host, ARMY_PRINT, Near(data, data.commander_title), division);
      }
    }
    Send(ds, host, ARMY_PRINT, Near(data, data.speaker_name_format), person);
    Send(ds, host, CONTEXT_PORTRAIT, 0x2d, 0x13c, person);
    Send(ds, host, UI_TEXT, Near(data, data.speaker_body_text));
    Send(ds, host, ARMY_POSITION, 0x36, 0x144);
    Send(ds, host, TEXT_SCROLL_MODE, 1);
    Send(ds, host, TEXT_WINDOW, 0x36, 0x4e, 0x142, 0x192);
  } else {
    Send(ds, host, TEXT_SCROLL_MODE, 0);
    Send(ds, host, TEXT_WINDOW_RESET);
    Send(ds, host, UI_TEXT, Near(data, data.speaker_end_text));
  }
  return 1;
}

// Puts the zero-terminated text at DS:text out character by character.
void opeu_text_feed(uint8_t ds[65536], const DialogHost* host, uint16_t text) {
  for (char c; (c = Record<char>(Data(ds), text)) != 0; ++text) Send(ds, host, TEXT_PUTC, static_cast<std::uint8_t>(c));
}

// The message spoken by the person, if the player sees the person.
void opeu_control_message(uint8_t ds[65536], const DialogHost* host, uint16_t text, uint16_t person) {
  if (opeu_speaker_panel(ds, host, person, 1)) {
    Send(ds, host, UI_TEXT, text);
    opeu_speaker_panel(ds, host, person, 0);
  }
}

// The prompt with its range ("(first-last)") and the value chosen.
uint16_t opeu_menu_prompt(uint8_t ds[65536], const DialogHost* host, uint16_t text, uint16_t first, uint16_t last,
                          uint16_t flag, uint16_t menu) {
  auto const& data{ Data(ds) };
  auto const  range{ host->scratch };
  Send(ds, host, REQUEST_FORMAT, range, Near(data, data.menu_prompt_format), first, last);
  opeu_text_feed(ds, host, text);
  opeu_text_feed(ds, host, range);
  return Send(ds, host, MENU_SELECT, first, last, flag, menu);
}

uint16_t opeu_control_choose(uint8_t ds[65536], const DialogHost* host, uint16_t text, uint16_t first, uint16_t last,
                             uint16_t flag, uint16_t person, uint16_t menu) {
  opeu_speaker_panel(ds, host, person, 1);
  auto const value{ opeu_menu_prompt(ds, host, text, first, last, flag, menu) };
  opeu_speaker_panel(ds, host, person, 0);
  return value;
}

uint16_t opeu_delay_dialog(uint8_t ds[65536], const DialogHost* host, uint16_t text, uint16_t last, uint16_t person,
                           uint16_t menu) {
  return opeu_control_choose(ds, host, text, 1, last, 1, person, menu);
}

uint16_t opeu_choose_dialog(uint8_t ds[65536], const DialogHost* host, uint16_t text, uint16_t last, uint16_t person,
                            uint16_t menu) {
  return opeu_control_choose(ds, host, text, 1, last, 0, person, menu);
}

uint16_t opeu_confirm_prompt(uint8_t ds[65536], const DialogHost* host, uint16_t text) {
  auto const& data{ Data(ds) };
  opeu_text_feed(ds, host, text);
  opeu_text_feed(ds, host, Near(data, data.confirm_space));
  return Send(ds, host, CONFIRM_INPUT);
}

uint16_t opeu_confirm_message(uint8_t ds[65536], const DialogHost* host, uint16_t text, uint16_t person) {
  opeu_speaker_panel(ds, host, person, 1);
  auto const answer{ opeu_confirm_prompt(ds, host, text) };
  opeu_speaker_panel(ds, host, person, 0);
  return answer;
}

uint16_t opeu_confirm_ok(uint8_t ds[65536], const DialogHost* host, uint16_t person) {
  auto const& data{ Data(ds) };
  return opeu_confirm_message(ds, host, Near(data, data.confirm_ok_text), person);
}

// "Wait n hours?" spoken by the person.
uint16_t opeu_delay_confirm(uint8_t ds[65536], const DialogHost* host, uint16_t person, uint8_t hours) {
  auto const& data{ Data(ds) };
  auto const  text{ host->scratch };
  if (hours == 1) return opeu_confirm_message(ds, host, Send(ds, host, REQUEST_TEXT, ONE_HOUR_TEXT), person);
  Send(ds, host, REQUEST_FORMAT, text, Near(data, data.delay_hours_format), hours);
  return opeu_confirm_message(ds, host, text, person);
}

// A report on the unit to its own side, spoken by its division commander, with the map moved to the unit.
void opeu_unit_report(uint8_t ds[65536], const DialogHost* host, uint16_t unit, uint16_t message) {
  auto const& data{ Data(ds) };
  auto const  text{ host->scratch };
  auto const  name{ static_cast<std::uint16_t>(host->scratch + 100) };
  auto const  side{ static_cast<std::uint8_t>(Send(ds, host, UNIT_SIDE, unit)) };
  if (data.side_status[side] == SIDE_PLAYED && Send(ds, host, SELECTION_ELIGIBLE, unit)) {
    auto const& record{ Record<Unit>(data, unit) };
    Send(ds, host, REQUEST_FORMAT, text, Near(data, data.unit_report_format), SideMark(side), data.side_names[side],
         Send(ds, host, UNIT_NAME, unit, name));
    Send(ds, host, STRING_APPEND, text, message);
    Send(ds, host, ARRIVAL_LOCATION, record.x, record.y, side);
    opeu_control_message(
        ds, host, text,
        Send(ds, host, FIND_GROUP_RECORD, side, static_cast<std::uint8_t>(Send(ds, host, GROUP_INDEX, unit))));
    Send(ds, host, ARRIVAL_VIEW, unit);
  }
}

// A report on the objective to its own side, spoken by the side's intelligence chief, with the objective flashed.
void opeu_objective_report(uint8_t ds[65536], const DialogHost* host, uint16_t objective, uint16_t message) {
  auto const& data{ Data(ds) };
  auto const  text{ host->scratch };
  auto const  side{ static_cast<std::uint8_t>(Send(ds, host, OBJECTIVE_SIDE, objective)) };
  if (data.side_status[side] == SIDE_PLAYED && Send(ds, host, OBJECTIVE_ELIGIBLE, objective)) {
    auto const& record{ Record<Objective>(data, objective) };
    Send(ds, host, REQUEST_FORMAT, text, Near(data, data.objective_report_format), SideMark(side), objective);
    Send(ds, host, STRING_APPEND, text, message);
    Send(ds, host, ARRIVAL_LOCATION, record.x, record.y, side);
    opeu_control_message(ds, host, text, Send(ds, host, FIND_SIDE_RECORD, side, INTELLIGENCE_CHIEF_KIND));
    Send(ds, host, ARRIVAL_FLASH, record.x, record.y, side);
  }
}

// A message about the unit: to the enemy's intelligence chief when the unit is special, else to its own division
// commander when the player sees it.
void opeu_unit_side_report(uint8_t ds[65536], const DialogHost* host, uint16_t unit, uint16_t message) {
  auto const&   data{ Data(ds) };
  std::uint16_t speaker;
  auto          side{ static_cast<std::uint8_t>(
      Send(ds, host, OPPOSITE_SIDE, static_cast<std::uint8_t>(Send(ds, host, UNIT_SIDE, unit)))) };
  if (data.side_status[side] == SIDE_PLAYED && Send(ds, host, UNIT_SPECIAL, unit))
    speaker = Send(ds, host, FIND_SIDE_RECORD, side, INTELLIGENCE_CHIEF_KIND);
  else {
    side = static_cast<std::uint8_t>(Send(ds, host, OPPOSITE_SIDE, side));
    if (data.side_status[side] == SIDE_PLAYED && Send(ds, host, SELECTION_ELIGIBLE, unit))
      speaker = Send(ds, host, FIND_GROUP_RECORD, side, static_cast<std::uint8_t>(Send(ds, host, GROUP_INDEX, unit)));
    else
      return;
  }
  auto const& record{ Record<Unit>(data, unit) };
  Send(ds, host, ARRIVAL_LOCATION, record.x, record.y, side);
  opeu_control_message(ds, host, message, speaker);
}

// A message about the objective to the enemy's intelligence chief, else to its own side's when the player sees it.
void opeu_objective_side_report(uint8_t ds[65536], const DialogHost* host, uint16_t objective, uint16_t message) {
  auto const& data{ Data(ds) };
  auto        side{ static_cast<std::uint8_t>(
      Send(ds, host, OPPOSITE_SIDE, static_cast<std::uint8_t>(Send(ds, host, OBJECTIVE_SIDE, objective)))) };
  if (data.side_status[side] != SIDE_PLAYED) {
    side = static_cast<std::uint8_t>(Send(ds, host, OPPOSITE_SIDE, side));
    if (data.side_status[side] != SIDE_PLAYED || !Send(ds, host, OBJECTIVE_ELIGIBLE, objective)) return;
  }
  auto const& record{ Record<Objective>(data, objective) };
  Send(ds, host, ARRIVAL_LOCATION, record.x, record.y, side);
  opeu_control_message(ds, host, message, Send(ds, host, FIND_SIDE_RECORD, side, INTELLIGENCE_CHIEF_KIND));
}

// The element type's panel: name, face and ratings (attack soft/hard, defense, speed, move range, gun range).
void opeu_element_type_panel(uint8_t ds[65536], const DialogHost* host, uint16_t type) {
  auto const& data{ Data(ds) };
  auto const& record{ Record<ElementType>(data, type) };
  auto const  index{ static_cast<std::int16_t>(static_cast<std::uint16_t>(type - Near(data, data.element_types)))
                     / static_cast<int>(sizeof(ElementType)) };
  opeu_army_title(ds, host, record.name);
  Send(ds, host, FACE_DRAW, 0x2d, 0xb4, 0, static_cast<std::uint8_t>(index));
  Send(ds, host, ARMY_POSITION, 0x44, 0xb4);
  Send(ds, host, ARMY_PRINT, Near(data, data.attack_format), record.hard_rating, record.soft_rating);
  Send(ds, host, ARMY_POSITION, 0x45, 0xc4);
  Send(ds, host, ARMY_PRINT, Near(data, data.defense_format), record.range);
  Send(ds, host, ARMY_POSITION, 0x45, 0xd4);
  Send(ds, host, ARMY_PRINT, Near(data, data.speed_format), record.speed);
  Send(ds, host, ARMY_POSITION, 0x45, 0xe4);
  Send(ds, host, ARMY_PRINT, Near(data, data.move_range_format),
       !record.category ? 0 : static_cast<std::uint16_t>(record.usage * record.weight) / 10);
  Send(ds, host, ARMY_POSITION, 0x45, 0xf4);
  Send(ds, host, ARMY_PRINT, Near(data, data.gun_range_format), record.mode);
}

// A battalion's panel: its type's name and face, quantity, morale, damage, training and experience.
void opeu_group_element_panel(uint8_t ds[65536], const DialogHost* host, uint16_t element) {
  auto const& data{ Data(ds) };
  auto const& record{ Record<Element>(data, element) };
  auto const  name{ host->scratch };
  opeu_army_title(ds, host, Send(ds, host, TYPE_NAME_TEXT, Near(data, data.element_types[record.type]), name));
  Send(ds, host, FACE_DRAW, 0x32, 0x9a, 0, record.type);
  Send(ds, host, ARMY_POSITION, 0x2e, 0xea);
  Send(ds, host, ARMY_PRINT, Near(data, data.group_element_format), record.quantity, record.morale, record.damage,
       record.training, record.experience);
}

// An equipment item's panel: its type's name and face, quantity and extra word.
void opeu_element_panel(uint8_t ds[65536], const DialogHost* host, uint16_t item) {
  auto const& data{ Data(ds) };
  auto const& record{ Record<Item>(data, item) };
  auto const  name{ host->scratch };
  opeu_army_title(ds, host, Send(ds, host, TYPE_NAME_TEXT, Near(data, data.element_types[record.type]), name));
  Send(ds, host, FACE_DRAW, 0x32, 0x9a, 0, record.type);
  Send(ds, host, ARMY_POSITION, 0x30, 0xea);
  Send(ds, host, ARMY_PRINT, Near(data, data.element_format), record.quantity, record.extra);
}

namespace {
constexpr std::uint16_t KEY_SCRATCH{ 16 }; // the keyboard routines' DS scratch
} // namespace

uint16_t svc_dialogs(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t r = 0, f;
  switch (id) {
  case BITMAP_RESTORE:
    r = (uint16_t)opeu_bitmap_restore(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2),
                                      Arg(args, count, 3), Arg(args, count, 4),
                                      port_far(Arg(args, count, 5), Arg(args, count, 6)));
    break;
  case BITMAP_SAVE:
    opeu_bitmap_save(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3),
                     port_far(Arg(args, count, 4), Arg(args, count, 5)));
    break;
  case UI_BOX:
    r = (uint16_t)opeu_ui_box(Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3),
                              Arg(args, count, 4));
    break;
  case VGA_CLEAR:       r = (uint16_t)opeu_vga_clear(); break;
  case DISPLAY_PREPARE: opeu_display_prepare(ds, svc_emsmap); break;
  case PICTURE_MEMORY:
    opeu_picture_memory_masked(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1),
                               Arg(args, count, 2), Arg(args, count, 3));
    break;
  case PALETTE_FOR_HOUR: opeu_palette_for_hour(ds, (const UiHost*)port_uiprim_host()); break;
  case PICTURE_FILE:
    opeu_picture_file(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1),
                      (int16_t)Arg(args, count, 2), Arg(args, count, 3));
    break;
  case UI_OUTLINE:
    opeu_ui_outline(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1),
                    Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4));
    break;
  case CONTEXT_FRAME:
    opeu_context_frame(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1),
                       Arg(args, count, 2));
    break;
  case CONTEXT_PORTRAIT:
    opeu_context_portrait(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1),
                          Arg(args, count, 2));
    break;
  case FACE_DRAW:
    opeu_face_draw(ds, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2),
                   (uint8_t)Arg(args, count, 3));
    break;
  case CONTEXT_SAVE:    opeu_context_save(ds, (TextHost*)port_textout_host()); break;
  case CONTEXT_RESTORE: opeu_context_restore(ds, (TextHost*)port_textout_host()); break;
  case ARMY_POSITION:
    opeu_army_position(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1));
    break;
  case UI_TEXT:        opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case ARMY_PRINT:     b1_cursor_print(ds, args, count); break;
  case REQUEST_FORMAT: b1_request_format(ds, args, count); break;
  case TEXT_SCROLL_MODE:
    r = (uint16_t)opeu_text_scroll_mode(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0));
    break;
  case TEXT_WINDOW:
    opeu_text_window(ds, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1),
                     (int16_t)Arg(args, count, 2), (int16_t)Arg(args, count, 3));
    break;
  case TEXT_WINDOW_RESET: opeu_text_window_reset(ds, (TextHost*)port_textout_host()); break;
  case TEXT_PUTC:         r = opeu_text_put(ds, (TextHost*)port_textout_host(), (uint8_t)Arg(args, count, 0)); break;
  case MENU_SELECT:
    f = b1_stack_alloc(KEY_SCRATCH);
    r = opeu_choose_value(ds, svc_keyboard, Arg(args, count, 0), Arg(args, count, 1), (int16_t)Arg(args, count, 2),
                          Arg(args, count, 3), f);
    port_scratch_leave(KEY_SCRATCH);
    break;
  case CONFIRM_INPUT:
    f = b1_stack_alloc(KEY_SCRATCH);
    r = (uint16_t)opeu_yes_no(ds, svc_keyboard, f);
    port_scratch_leave(KEY_SCRATCH);
    break;
  case WAIT_TICKS:         opeu_delay_units(ds, svc_waitkey, (uint8_t)Arg(args, count, 0)); break;
  case MUSIC_COMMAND7:     opeu_music_command7(ds, svc_sound); break;
  case SOUND_CUE16:        opeu_sound_cue16(ds, svc_soundcue); break;
  case LACKS_SIDE0:        r = opeu_person_lacks_side0_flag(ds, Arg(args, count, 0)); break;
  case SELECTED_UNIT:      r = opeu_person_selected_unit(ds, Arg(args, count, 0)); break;
  case SELECTION_ELIGIBLE: r = opeu_unit_selection_eligible(ds, Arg(args, count, 0)); break;
  case DAILY_CONTEXT:      opeu_daily_context(ds, svc_context, (uint8_t)Arg(args, count, 0)); break;
  case STRING_LENGTH:      r = opeu_string_length(ds, Arg(args, count, 0)); break;
  case STRING_APPEND:      r = opeu_string_append(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case REQUEST_TEXT:       r = opeu_message_lookup(ds, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  case UNIT_SIDE:          r = opeu_unit_side(Arg(args, count, 0)); break;
  case UNIT_NAME:          r = opeu_unit_name(ds, svc_unitname, Arg(args, count, 0), Arg(args, count, 1)); break;
  case ARRIVAL_LOCATION:
    f = b1_frame_enter();
    opeu_arrival_location(ds, svc_mappos, f, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1),
                          (uint8_t)Arg(args, count, 2));
    port_frame_leave();
    break;
  case GROUP_INDEX: r = opeu_unit_group_index(Arg(args, count, 0)); break;
  case FIND_GROUP_RECORD:
    r = opeu_find_group_record(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
    break;
  case ARRIVAL_VIEW:
    f = b1_frame_enter();
    opeu_arrival_view(ds, svc_mapunit, f, Arg(args, count, 0));
    port_frame_leave();
    break;
  case OBJECTIVE_SIDE:     r = opeu_objective_side(ds, Arg(args, count, 0)); break;
  case OBJECTIVE_ELIGIBLE: r = opeu_objective_eligible(ds, Arg(args, count, 0)); break;
  case FIND_SIDE_RECORD:
    r = opeu_find_side_record(ds, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1));
    break;
  case ARRIVAL_FLASH:
    f = b1_frame_enter();
    opeu_side_cell_flash(ds, svc_mapunit, f, (uint8_t)Arg(args, count, 0), (uint8_t)Arg(args, count, 1),
                         (uint8_t)Arg(args, count, 2));
    port_frame_leave();
    break;
  case OPPOSITE_SIDE:  r = opeu_opposite_side((uint8_t)Arg(args, count, 0)); break;
  case UNIT_SPECIAL:   r = opeu_unit_special(ds, Arg(args, count, 0)); break;
  case TYPE_NAME_TEXT: r = opeu_type_name(ds, Arg(args, count, 0), Arg(args, count, 1)); break;
  case PRESENT: r = (uint16_t)port_call(ds, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0), 0, 0); break;
  default:      port_unhandled("dialogs", id, count);
  }
  return r;
}

auto port_routine_opeu_army_title(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_army_title(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_confirm_ok(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_confirm_ok(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0));
}

auto port_routine_opeu_control_choose(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_control_choose(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 2), Arg(args, count, 0),
                             Arg(args, count, 1), Arg(args, count, 3), Arg(args, count, 4), Arg(args, count, 5));
}

auto port_routine_opeu_control_message(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_control_message(ds, (const DialogHost*)port_dialogs_host(), Arg(args, count, 1), Arg(args, count, 0));
  return 0;
}

auto port_routine_opeu_info_panel_begin(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_info_panel_begin(ds, (const DialogHost*)port_dialogs_host());
  return 0;
}

auto port_routine_opeu_screen_reset(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_screen_reset(ds, (const DialogHost*)port_dialogs_host());
  return 0;
}

auto port_routine_opeu_title_panel_begin(std::uint8_t* ds, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_title_panel_begin(ds, (const DialogHost*)port_dialogs_host());
  return 0;
}
