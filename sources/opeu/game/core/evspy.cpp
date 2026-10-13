// The commander's evening visitor in scenario 4 (side 0 under the player's command). At 15:00 the adviser announces
// her; accepting shows her greeting (charm rises), declining ends the event for the day. At 20:00 a second offer may
// follow: the approach scene asks a yes/no question, and the accepted ending raises the commander's experience, lowers
// his health limit and caps his health at it. The commander must be fit (charm and health 70 or more, experience 50
// or more), the weather calm and his unit safe.
#include "core/evspy.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/perssel.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "common/scenario.hpp"
#include "core/dialogs.hpp"
#include "core/evvisit.hpp"
#include "core/msglook.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "io/sound.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

// The services the event calls through its event.
enum Service : std::uint16_t {
  SAFE     = 0,  // (unit) -> whether the commander's unit is safe for a visit
  PANEL    = 1,  // () the message panel
  FRAME    = 2,  // (x, y, width)
  PRINT    = 3,  // (format)
  SEEK     = 4,  // (handle, offset low, offset high, origin) in the portrait file
  DRAW     = 5,  // (x, y, width, height, mode, handle) the portrait read from the file
  POSITION = 6,  // (x, y)
  FORMAT   = 7,  // (buffer, format[, argument])
  TEXT     = 8,  // (text) into the message panel
  UI_TEXT  = 9,  // (text) straight to the screen
  ANSWER   = 10, // () -> yes (1) or no (0)
  CONTEXT  = 11, // (value) the turn context
  FINISH   = 12, // (value) the side's initialization
  SHOW     = 13, // (id, side, presenter offset, presenter segment)
  MESSAGE  = 14, // (text, person)
  ASK      = 15, // (text, person) -> the answer
  LOOKUP   = 16, // (id) -> the message text
};

constexpr std::uint16_t BUFFER{ offsetof(DataSegment, event_buffer) };
constexpr std::uint16_t NAME_FORMAT{ offsetof(DataSegment, spy_name_format) };
constexpr std::uint16_t LAUGH{ offsetof(DataSegment, spy_laugh_text) };
constexpr std::uint16_t MOVE{ offsetof(DataSegment, spy_move_text) };
constexpr std::uint16_t BOLD_OFF{ offsetof(DataSegment, spy_bold_off) };
constexpr std::uint16_t HUMPH{ offsetof(DataSegment, spy_humph_text) };
constexpr std::uint16_t PORTRAIT_LOW{ 0x7500 }; // her portrait's position in the portrait file
constexpr std::uint16_t PORTRAIT_HIGH{ 3 };
constexpr std::uint16_t SPY_CONTEXT{ 0x11 };
constexpr std::uint16_t CODE_SEGMENT{ 0x108c }; // the presenters' code segment
constexpr std::uint16_t PRESENT_A_OFFSET{ 0x10b8 };
constexpr std::uint16_t PRESENT_B_OFFSET{ 0x1132 };
constexpr std::uint16_t GREETING_SHOW{ 0xba };
constexpr std::uint16_t ENDING_SHOW{ 0xca };
constexpr Scenario      SPY_SCENARIO{ Scenario::BULGE };
constexpr std::uint8_t  COMMANDER{ 1 };
constexpr std::uint8_t  ADVISER{ 2 };
constexpr std::uint8_t  OFFER_HOUR{ 15 };
constexpr std::uint8_t  APPROACH_HOUR{ 20 };
constexpr std::uint8_t  FIT{ 70 };
constexpr std::uint8_t  EXPERIENCED{ 50 };

auto Lookup(Game& game, HostEvent event, std::uint16_t id) -> std::uint16_t {
  return Call(game, event, LOOKUP, id);
}
auto Commander(Game& game) -> std::uint16_t {
  return opeu_find_side_record(game, 0, COMMANDER);
}
} // namespace

// Whether side 0's commander is fit for the visit, the weather calm (clear or 1) and his unit safe.
uint16_t opeu_spy_commander_ready(Game& game, HostEvent event) {
  auto&       data{ Data(game) };
  auto const  near{ Commander(game) };
  auto const  unit{ opeu_person_selected_unit(game, near) };
  auto const& commander{ Record<Person>(data, near) };
  return (data.weather == 0 || data.weather == 1) && commander.charm >= FIT && commander.health >= FIT && commander.experience >= EXPERIENCED && Call(game, event, SAFE, unit) ? 1 : 0;
}

// The message panel with her name and, when `show`, her portrait; then the text.
void opeu_spy_portrait_text(Game& game, HostEvent event, uint16_t text, uint16_t show) {
  auto const& data{ Data(game) };
  Call(game, event, PANEL);
  Call(game, event, FRAME, 44, 282, 15);
  Call(game, event, PRINT, Lookup(game, event, 0x2b));
  if (show) {
    Call(game, event, SEEK, data.portrait_handle, PORTRAIT_LOW, PORTRAIT_HIGH, 0);
    Call(game, event, DRAW, 45, 316, 8, 80, 0, data.portrait_handle);
  }
  Call(game, event, POSITION, 54, 324);
  Call(game, event, FORMAT, BUFFER, NAME_FORMAT, text);
  Call(game, event, TEXT, BUFFER);
}

// The greeting scene (messages 44..51); the commander's charm rises by 1.
void opeu_spy_present_a(Game& game, HostEvent event) {
  Call(game, event, CONTEXT, SPY_CONTEXT);
  auto const near{ Commander(game) };
  for (std::uint16_t id{ 0x2c }; id <= 0x33; ++id) {
    Call(game, event, FORMAT, BUFFER, Lookup(game, event, id), near);
    opeu_spy_portrait_text(game, event, BUFFER, 1);
  }
  Call(game, event, FINISH, 0);
  opeu_add_byte(1, RATING_LIMIT, &Record<Person>(Data(game), near).charm);
}

void opeu_spy_show_a(Game& game, HostEvent event) {
  Call(game, event, SHOW, GREETING_SHOW, 0, PRESENT_A_OFFSET, CODE_SEGMENT);
}

// The accepted ending: experience up by 1, the health limit down by 1 and health capped at it.
void opeu_spy_present_b(Game& game, HostEvent event) {
  Call(game, event, FORMAT, BUFFER, LAUGH);
  opeu_spy_portrait_text(game, event, BUFFER, 1);
  auto& commander{ Record<Person>(Data(game), Commander(game)) };
  opeu_add_byte(1, RATING_LIMIT, &commander.experience);
  opeu_sub_byte(1, &commander.max_health);
  commander.health = static_cast<std::uint8_t>(opeu_min16(commander.health, commander.max_health));
}

// The approach scene and its question.
void opeu_spy_approach(Game& game, HostEvent event) {
  auto const& data{ Data(game) };
  Call(game, event, CONTEXT, SPY_CONTEXT);
  Call(game, event, FORMAT, BUFFER, Lookup(game, event, 0x6b));
  opeu_spy_portrait_text(game, event, BUFFER, 1);
  Call(game, event, FORMAT, BUFFER, Lookup(game, event, 0x6c));
  opeu_spy_portrait_text(game, event, BUFFER, 1);
  Call(game, event, PANEL);
  if (data.restart_flag == 1) {
    Call(game, event, TEXT, MOVE);
    auto const accepted{ Call(game, event, ANSWER) };
    Call(game, event, UI_TEXT, BOLD_OFF);
    if (accepted)
      Call(game, event, SHOW, ENDING_SHOW, 0, PRESENT_B_OFFSET, CODE_SEGMENT);
    else {
      Call(game, event, FORMAT, BUFFER, HUMPH);
      opeu_spy_portrait_text(game, event, BUFFER, 1);
    }
  }
  Call(game, event, FINISH, 0);
}

// The hourly check (SideFlag::SPY_SENT: the offer was taken, SPY_DONE: the event is over for the day).
void opeu_spy_event(Game& game, HostEvent event) {
  auto& data{ Data(game) };
  if (CurrentScenario(data) != SPY_SCENARIO || data.side_control[0] != SIDE_PLAYED) return;
  auto const adviser{ opeu_find_side_record(game, 0, ADVISER) };
  auto&      flags{ data.side_flags[0] };
  auto const sent{ Bits(SideFlag::SPY_SENT) };
  auto const over{ static_cast<std::uint8_t>(Bits(SideFlag::SPY_SENT) | Bits(SideFlag::SPY_DONE)) };
  if (data.hour == OFFER_HOUR && !(flags & sent) && opeu_spy_commander_ready(game, event)) {
    Call(game, event, MESSAGE, Lookup(game, event, 0x38), adviser);
    if (Call(game, event, ASK, Lookup(game, event, 0x39), adviser)) {
      opeu_spy_show_a(game, event);
      flags |= sent;
    } else
      flags |= over;
  }
  if (data.hour == APPROACH_HOUR && (flags & over) == sent && opeu_spy_commander_ready(game, event)) {
    if (Call(game, event, ASK, Lookup(game, event, 0x3a), adviser)) opeu_spy_approach(game, event);
    flags |= Bits(SideFlag::SPY_DONE);
  }
}

uint16_t svc_evspy(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  uint16_t result{ 0 };
  switch (id) {
  case SAFE:  result = opeu_visit_unit_safe(game, Arg(args, count, 0)); break;
  case PANEL: opeu_message_panel_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case FRAME: opeu_context_frame(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case PRINT: opeu_cursor_print(game, { args, count }); break;
  case SEEK:  result = static_cast<uint16_t>(opeu_dos_seek((int16_t)Arg(args, count, 0), (int32_t)Arg32(args, count, 1), (int16_t)Arg(args, count, 3))); break;
  case DRAW:
    opeu_image_read_draw(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3), Arg(args, count, 4),
                         (int16_t)Arg(args, count, 5));
    break;
  case POSITION: opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case FORMAT:   opeu_request_format(game, { args, count }); break;
  case TEXT:     opeu_text_feed(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case UI_TEXT:  opeu_ui_text(game, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case ANSWER:   result = static_cast<uint16_t>(opeu_yes_no(game, svc_keyboard)); break;
  case CONTEXT:  opeu_set_turn_context(game, svc_sound, static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  case FINISH:   opeu_finish_side_initialization(game, svc_sound, static_cast<std::uint8_t>(Arg(args, count, 0))); break;
  case SHOW:     port_daily_show(game, Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2), Arg(args, count, 3)); break;
  case MESSAGE:  opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case ASK:      result = opeu_confirm_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case LOOKUP:   result = opeu_message_lookup(game, svc_msglook, (int16_t)Arg(args, count, 0)); break;
  default:       port_unhandled("evspy", id, count);
  }
  return result;
}

auto port_routine_opeu_spy_event(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_spy_event(game, svc_evspy);
  return 0;
}

auto port_routine_opeu_spy_present_a(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_spy_present_a(game, svc_evspy);
  return 0;
}

auto port_routine_opeu_spy_present_b(Game& game, std::uint16_t const*, unsigned) -> std::uint32_t {
  opeu_spy_present_b(game, svc_evspy);
  return 0;
}
