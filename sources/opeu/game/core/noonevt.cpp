// The dated noon events: at 12:00 each event of the day shows its news, runs its action (blocking or allowing the
// Axis requests, raising or lowering a side's morale, readying reserves, adding to the Allied pool) and may have a
// leader speak.
#include "types.hpp"
#include "core/noonevt.hpp"
#include "core/memory.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/strings.hpp"
#include "types/events.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "data-segment.hpp"

#include <cstddef>
#include <span>
#include <utility>

#include "port.hpp"
#include "glue.hpp"
#include "services.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "io/soundcue.hpp"

using namespace opeu::game;

namespace {

// The services the noon events call through their event.
enum NoonService : uint16_t {
  NOON_PANEL         = 0, // open the message panel
  NOON_UI_TEXT       = 1, // (text)
  NOON_FRAME         = 2, // (x, y, width): the speaker's frame
  NOON_PRINT         = 3, // (format, argument)
  NOON_PORTRAIT      = 4, // (x, y, person)
  NOON_POSITION      = 5, // (x, y): the text position
  NOON_WAIT          = 6, // (value): wait for a key
  NOON_FEED          = 7, // (text): show the speech
  NOON_DAILY_CONTEXT = 8, // (side)
  NOON_ACTIVE_BEGIN  = 9,
  NOON_SOUND         = 10, // the news cue
  NOON_MESSAGE       = 11, // (text, person)
  NOON_FAR_ACTION    = 12, // (offset, segment): an action outside this module
};

// The code segment of the noon actions, and each action's offset in it, as the event records name them.
using NoonAction = void (*)(uint8_t*, const uint8_t*, HostEvent, uint16_t);
struct ActionEntry {
  uint16_t   offset;
  NoonAction run;
};
constexpr ActionEntry NOON_ACTIONS[]{
  { 0x0004, opeu_noon_no_action },
  { 0x0006, opeu_noon_block_axis_regts },
  { 0x000e, opeu_noon_block_axis_sup_bns },
  { 0x0018, opeu_noon_block_axis_bns_regts },
  { 0x0022, opeu_noon_allow_axis_bns_regts },
  { 0x002c, opeu_noon_raise_axis_morale },
  { 0x0066, opeu_noon_lower_axis_morale },
  { 0x009c, opeu_noon_lower_allied_morale },
  { 0x00d2, opeu_noon_axis_morale_blockade },
  { 0x00dc, opeu_code_0e57_00dc },
  { 0x010e, opeu_code_0e57_010e },
  { 0x0140, opeu_code_0e57_0140 },
};

// NoonEvent::leader & AUDIENCE_MASK: who hears the speech; 0 for no one.
enum Audience : uint8_t {
  NOT_ALLIED_SOLO =
      1, // not a player commanding side 1 alone against the computer
  NOT_AXIS_SOLO =
      2, // not a player commanding side 0 alone against the computer
  EVERYONE = 3,
};
constexpr uint8_t AUDIENCE_MASK{ 0x03 };
constexpr uint8_t NO_SPEECH{ 0xff };
constexpr uint8_t END_OF_EVENTS{ 0xff };

// The DS stack the speech and the news use below the caller's SP.
constexpr uint16_t SPEECH_BUFFER_AT{ 0x76 }; // 110 bytes
constexpr uint16_t SPEECH_SP{ 0x7e };
constexpr uint16_t NEWS_BUFFER_AT{ 0x64 }; // 90 bytes
constexpr uint16_t LEADER_SP{ 0x6c };
constexpr uint16_t FAR_CALL{ 4 }; // a far call pushes CS:IP

auto Event(uint8_t* ds, HostEvent event, NoonService service, uint16_t count,
           uint16_t a = 0, uint16_t b = 0, uint16_t c = 0) -> uint16_t {
  uint16_t args[]{ a, b, c };
  return event(ds, service, args, count);
}

void SetRequests(uint8_t* ds, RequestKind kind, uint16_t enabled) {
  Data(ds).special[0].enabled[std::to_underlying(kind)] = enabled;
}

// Each element of the side gains (raise) or loses 30..49 morale, up to 99.
void ShiftMorale(uint8_t* ds, uint8_t side, bool raise) {
  for (auto& element : std::span{ Data(ds).elements }.subspan(
           side * ELEMENTS_PER_SIDE, ELEMENTS_PER_SIDE)) {
    auto const amount{ static_cast<uint16_t>(RandomBelow(ds, 20) + 30) };
    if (raise)
      opeu_add_byte(amount, 99, &element.morale);
    else
      opeu_sub_byte(amount, &element.morale);
  }
}

// A side with a reserve division expects its reserves.
void ExpectReserves(uint8_t* ds, uint8_t side) {
  auto& data{ Data(ds) };
  for (uint8_t i = 0; i < GROUPS_PER_SIDE; i++)
    if (data.groups[side * GROUPS_PER_SIDE + i].flags &
        Bits(GroupFlag::RESERVE)) {
      data.side_flags[side] |= Bits(SideFlag::RESERVES_PENDING);
      break;
    }
}

// Runs the event's action: an action of this module directly, any other through the service.
void RunAction(uint8_t* ds, const uint8_t* far_text, HostEvent event,
               uint16_t sp, uint16_t code_segment, FarPointer const& action) {
  if (action.segment == code_segment)
    for (auto const& entry : NOON_ACTIONS)
      if (entry.offset == action.offset) {
        entry.run(ds, far_text, event, static_cast<uint16_t>(sp - FAR_CALL));
        return;
      }
  Event(ds, event, NOON_FAR_ACTION, 2, action.offset, action.segment);
}

constexpr auto At(std::size_t offset) -> uint16_t {
  return static_cast<uint16_t>(offset);
}

} // namespace

void opeu_noon_no_action(uint8_t*, const uint8_t*, HostEvent, uint16_t) {}

void opeu_noon_block_axis_regts(uint8_t ds[65536], const uint8_t*, HostEvent,
                                uint16_t) {
  SetRequests(ds, RequestKind::REGIMENT, 0);
}

void opeu_noon_block_axis_sup_bns(uint8_t ds[65536], const uint8_t*, HostEvent,
                                  uint16_t) {
  SetRequests(ds, RequestKind::SUPPLIES, 0);
  SetRequests(ds, RequestKind::BATTALION, 0);
}

void opeu_noon_block_axis_bns_regts(uint8_t ds[65536], const uint8_t*,
                                    HostEvent, uint16_t) {
  SetRequests(ds, RequestKind::BATTALION, 0);
  SetRequests(ds, RequestKind::REGIMENT, 0);
}

void opeu_noon_allow_axis_bns_regts(uint8_t ds[65536], const uint8_t*,
                                    HostEvent, uint16_t) {
  SetRequests(ds, RequestKind::BATTALION, 1);
  SetRequests(ds, RequestKind::REGIMENT, 1);
}

void opeu_noon_raise_axis_morale(uint8_t ds[65536], const uint8_t*, HostEvent,
                                 uint16_t) {
  ShiftMorale(ds, 0, true);
}

void opeu_noon_lower_axis_morale(uint8_t ds[65536], const uint8_t*, HostEvent,
                                 uint16_t) {
  ShiftMorale(ds, 0, false);
}

void opeu_noon_lower_allied_morale(uint8_t ds[65536], const uint8_t*, HostEvent,
                                   uint16_t) {
  ShiftMorale(ds, 1, false);
}

void opeu_noon_axis_morale_blockade(uint8_t ds[65536], const uint8_t* far_text,
                                    HostEvent event, uint16_t sp) {
  opeu_noon_lower_axis_morale(ds, far_text, event, sp);
  opeu_noon_block_axis_sup_bns(ds, far_text, event, sp);
}

// The Axis expects reserves when it has a reserve division.
void opeu_code_0e57_00dc(uint8_t ds[65536], const uint8_t*, HostEvent,
                         uint16_t) {
  ExpectReserves(ds, 0);
}

// The Allies expect reserves when they have a reserve division.
void opeu_code_0e57_010e(uint8_t ds[65536], const uint8_t*, HostEvent,
                         uint16_t) {
  ExpectReserves(ds, 1);
}

// The Allied pool gains 500, up to 3000.
void opeu_code_0e57_0140(uint8_t ds[65536], const uint8_t*, HostEvent,
                         uint16_t) {
  opeu_add_word(500, 3000, &Data(ds).sides[1].pool_a);
}

// Opens (show 1) the panel with the person's post, name and portrait, or closes it and waits for a key.
uint16_t opeu_noon_portrait(uint8_t ds[65536], const uint8_t*, HostEvent event,
                            uint16_t, uint16_t person, uint16_t show) {
  auto& data{ Data(ds) };
  if (show == 1) {
    Event(ds, event, NOON_PANEL, 0);
    Event(ds, event, NOON_UI_TEXT, 1,
          At(offsetof(DataSegment, noon_role_style)));
    auto const role{ data.role_names[(Record<Person>(data, person).role &
                                      Bits(PersonRole::ROLE_MASK)) -
                                     1] };
    Event(ds, event, NOON_FRAME, 3, 0x2c, 0x11a,
          static_cast<uint8_t>(opeu_string_length(ds, person) +
                               opeu_string_length(ds, role) + 5));
    Event(ds, event, NOON_UI_TEXT, 1, role);
    Event(ds, event, NOON_PRINT, 2, At(offsetof(DataSegment, noon_name_format)),
          person);
    Event(ds, event, NOON_PORTRAIT, 3, 0x2d, 0x13c, person);
    Event(ds, event, NOON_UI_TEXT, 1,
          At(offsetof(DataSegment, noon_speech_style)));
    Event(ds, event, NOON_POSITION, 2, 0x36, 0x144);
  } else {
    Event(ds, event, NOON_UI_TEXT, 1,
          At(offsetof(DataSegment, noon_speech_end)));
    Event(ds, event, NOON_WAIT, 1, 1);
  }
  return 1;
}

// The person speaks the text in the portrait panel.
void opeu_noon_speech(uint8_t ds[65536], const uint8_t* far_text,
                      HostEvent event, uint16_t sp, uint16_t speech,
                      uint16_t person) {
  if (opeu_noon_portrait(ds, far_text, event, sp, person, 1)) {
    Event(ds, event, NOON_FEED, 1, speech);
    opeu_noon_portrait(ds, far_text, event, sp, person, 0);
  }
}

// The person template the code names speaks noon_speeches[line] when the audience hears it.
void opeu_noon_leader(uint8_t ds[65536], const uint8_t* far_text,
                      HostEvent event, uint16_t sp, uint8_t code,
                      uint8_t line) {
  auto const& data{ Data(ds) };
  auto        audience{ static_cast<uint8_t>(code & AUDIENCE_MASK) };
  auto const  allied_solo{ data.side_control[0] == 0 &&
                           data.side_control[1] == SIDE_PLAYED };
  auto const  axis_solo{ data.side_control[0] == SIDE_PLAYED &&
                         data.side_control[1] == 0 };
  if ((audience == NOT_ALLIED_SOLO && !allied_solo) ||
      (audience == NOT_AXIS_SOLO && !axis_solo))
    audience = EVERYONE;
  if (audience != EVERYONE)
    return;
  auto const speech{ opeu_string_copy_far(ds, Local(sp, SPEECH_BUFFER_AT),
                                          const_cast<uint8_t*>(far_text),
                                          data.noon_speeches[line].offset) };
  opeu_noon_speech(ds, far_text, event, Local(sp, SPEECH_SP), speech,
                   At(PERSON_TEMPLATES_AT + sizeof(Person) * (code >> 2)));
}

// The day's noon events (at 12:00).
void opeu_turn_noon_events(uint8_t ds[65536], const uint8_t* far_text,
                           HostEvent event, uint16_t sp,
                           uint16_t code_segment) {
  auto&      data{ Data(ds) };
  auto const news{ Local(sp, NEWS_BUFFER_AT) };
  Event(ds, event, NOON_DAILY_CONTEXT, 1, data.current_side);
  for (auto const& noon : data.noon_events) {
    if (noon.date == END_OF_EVENTS)
      break;
    if (noon.date >> 4 != data.year || (noon.date & 0x0f) != data.month ||
        noon.day != data.day)
      continue;
    if (noon.action.offset | noon.action.segment)
      RunAction(ds, far_text, event, sp, code_segment, noon.action);
    Event(ds, event, NOON_ACTIVE_BEGIN, 0);
    opeu_string_copy_far(ds, news, const_cast<uint8_t*>(far_text),
                         noon.news.offset);
    opeu_string_append(ds, news, At(offsetof(DataSegment, noon_news_end)));
    if (far_text[noon.news.offset])
      Event(ds, event, NOON_SOUND, 0);
    Event(ds, event, NOON_MESSAGE, 2, news,
          opeu_find_side_record(
              ds, data.current_side,
              std::to_underlying(StaffRole::INTELLIGENCE_CHIEF)));
    if (noon.leader != NO_SPEECH)
      opeu_noon_leader(ds, far_text, event, Local(sp, LEADER_SP), noon.leader,
                       noon.line);
  }
}

// The services of the noon events (NoonService).
uint16_t svc_noonevt(uint8_t* ds, uint16_t id, const uint16_t* args,
                     uint16_t count) {
  switch (id) {
    case NOON_PANEL:
      opeu_message_panel_begin(ds, (const DialogHost*)port_dialogs_host());
      break;
    case NOON_UI_TEXT:
      opeu_ui_text(ds, (TextHost*)port_textout_host(), Arg(args, count, 0));
      break;
    case NOON_FRAME:
      opeu_context_frame(ds, (const UiHost*)port_uiprim_host(),
                         Arg(args, count, 0), Arg(args, count, 1),
                         Arg(args, count, 2));
      break;
    case NOON_PRINT: c1_cursor_print(ds, args, count); break;
    case NOON_PORTRAIT:
      opeu_context_portrait(ds, (const UiHost*)port_uiprim_host(),
                            Arg(args, count, 0), Arg(args, count, 1),
                            Arg(args, count, 2));
      break;
    case NOON_POSITION:
      opeu_army_position(ds, (TextHost*)port_textout_host(),
                         (int16_t)Arg(args, count, 0),
                         (int16_t)Arg(args, count, 1));
      break;
    case NOON_WAIT:
      opeu_wait_confirm(ds, svc_keyboard, (int16_t)Arg(args, count, 0));
      break;
    case NOON_FEED:
      opeu_text_feed(ds, (const DialogHost*)port_dialogs_host(),
                     Arg(args, count, 0));
      break;
    case NOON_DAILY_CONTEXT:
      opeu_daily_context(ds, svc_context, (uint8_t)Arg(args, count, 0));
      break;
    case NOON_ACTIVE_BEGIN:
      opeu_turn_active_begin(ds, (const DialogHost*)port_dialogs_host());
      break;
    case NOON_SOUND: opeu_sound_cue10(ds, svc_soundcue); break;
    case NOON_MESSAGE:
      opeu_control_message(ds, (const DialogHost*)port_dialogs_host(),
                           Arg(args, count, 0), Arg(args, count, 1));
      break;
    case NOON_FAR_ACTION:
      port_call(ds, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0),
                nullptr, 0);
      break;
    default: port_unhandled("noonevt", id, count);
  }
  return 0;
}
