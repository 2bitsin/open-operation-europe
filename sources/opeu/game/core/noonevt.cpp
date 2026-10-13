// The dated noon events: at 12:00 each event of the day shows its news, runs its action (blocking or allowing the
// Axis requests, raising or lowering a side's morale, readying reserves, adding to the Allied pool) and may have a
// leader speak.
#include "core/noonevt.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "common/random.hpp"
#include "core/memory.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/strings.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include <cstddef>
#include <span>
#include <utility>

#include "common/game.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "io/soundcue.hpp"
#include "link/glue.hpp"
#include "link/link.hpp"
#include "port.hpp"
#include "services.hpp"

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
using NoonAction = void (*)(Game&, const uint8_t*, HostEvent);
struct ActionEntry {
  uint16_t   offset;
  NoonAction run;
};
constexpr ActionEntry NOON_ACTIONS[]{
  { 0x0004, opeu_noon_no_action            },
  { 0x0006, opeu_noon_block_axis_regts     },
  { 0x000e, opeu_noon_block_axis_sup_bns   },
  { 0x0018, opeu_noon_block_axis_bns_regts },
  { 0x0022, opeu_noon_allow_axis_bns_regts },
  { 0x002c, opeu_noon_raise_axis_morale    },
  { 0x0066, opeu_noon_lower_axis_morale    },
  { 0x009c, opeu_noon_lower_allied_morale  },
  { 0x00d2, opeu_noon_axis_morale_blockade },
  { 0x00dc, opeu_code_0e57_00dc            },
  { 0x010e, opeu_code_0e57_010e            },
  { 0x0140, opeu_code_0e57_0140            },
};

// NoonEvent::leader & AUDIENCE_MASK: who hears the speech; 0 for no one.
enum Audience : uint8_t {
  NOT_ALLIED_SOLO = 1, // not a player commanding side 1 alone against the computer
  NOT_AXIS_SOLO   = 2, // not a player commanding side 0 alone against the computer
  EVERYONE        = 3,
};
constexpr uint8_t AUDIENCE_MASK{ 0x03 };
constexpr uint8_t NO_SPEECH{ 0xff };
constexpr uint8_t END_OF_EVENTS{ 0xff };

// The texts the noon events hand to services.
constexpr uint16_t SPEECH_BYTES{ 110 }; // the speech the feed service shows
constexpr uint16_t NEWS_BYTES{ 90 };    // the news the message service shows

auto Event(Game& game, HostEvent event, NoonService service, uint16_t count, uint16_t a = 0, uint16_t b = 0, uint16_t c = 0) -> uint16_t {
  uint16_t args[]{ a, b, c };
  return event(game, service, args, count);
}

void SetRequests(Game& game, RequestKind kind, uint16_t enabled) {
  Data(game).special[0].enabled[std::to_underlying(kind)] = enabled;
}

// Each element of the side gains (raise) or loses 30..49 morale, up to 99.
void ShiftMorale(Game& game, uint8_t side, bool raise) {
  for (auto& element : std::span{ Data(game).elements }.subspan(side * ELEMENTS_PER_SIDE, ELEMENTS_PER_SIDE)) {
    auto const amount{ static_cast<uint16_t>(RandomBelow(game, 20) + 30) };
    if (raise)
      opeu_add_byte(amount, 99, &element.morale);
    else
      opeu_sub_byte(amount, &element.morale);
  }
}

// A side with a reserve division expects its reserves.
void ExpectReserves(Game& game, uint8_t side) {
  auto& data{ Data(game) };
  for (uint8_t i = 0; i < GROUPS_PER_SIDE; i++)
    if (GroupOf(data, side, i).flags & Bits(GroupFlag::RESERVE)) {
      data.side_flags[side] |= Bits(SideFlag::RESERVES_PENDING);
      break;
    }
}

// Runs the event's action: an action of this module directly, any other through the service.
void RunAction(Game& game, const uint8_t* far_text, HostEvent event, uint16_t code_segment, FarPointer const& action) {
  if (action.segment == code_segment)
    for (auto const& entry : NOON_ACTIONS)
      if (entry.offset == action.offset) {
        entry.run(game, far_text, event);
        return;
      }
  Event(game, event, NOON_FAR_ACTION, 2, action.offset, action.segment);
}

constexpr auto At(std::size_t offset) -> uint16_t {
  return static_cast<uint16_t>(offset);
}

} // namespace

void opeu_noon_no_action(Game&, const uint8_t*, HostEvent) { }

void opeu_noon_block_axis_regts(Game& game, const uint8_t*, HostEvent) {
  SetRequests(game, RequestKind::REGIMENT, 0);
}

void opeu_noon_block_axis_sup_bns(Game& game, const uint8_t*, HostEvent) {
  SetRequests(game, RequestKind::SUPPLIES, 0);
  SetRequests(game, RequestKind::BATTALION, 0);
}

void opeu_noon_block_axis_bns_regts(Game& game, const uint8_t*, HostEvent) {
  SetRequests(game, RequestKind::BATTALION, 0);
  SetRequests(game, RequestKind::REGIMENT, 0);
}

void opeu_noon_allow_axis_bns_regts(Game& game, const uint8_t*, HostEvent) {
  SetRequests(game, RequestKind::BATTALION, 1);
  SetRequests(game, RequestKind::REGIMENT, 1);
}

void opeu_noon_raise_axis_morale(Game& game, const uint8_t*, HostEvent) {
  ShiftMorale(game, 0, true);
}

void opeu_noon_lower_axis_morale(Game& game, const uint8_t*, HostEvent) {
  ShiftMorale(game, 0, false);
}

void opeu_noon_lower_allied_morale(Game& game, const uint8_t*, HostEvent) {
  ShiftMorale(game, 1, false);
}

void opeu_noon_axis_morale_blockade(Game& game, const uint8_t* far_text, HostEvent event) {
  opeu_noon_lower_axis_morale(game, far_text, event);
  opeu_noon_block_axis_sup_bns(game, far_text, event);
}

// The Axis expects reserves when it has a reserve division.
void opeu_code_0e57_00dc(Game& game, const uint8_t*, HostEvent) {
  ExpectReserves(game, 0);
}

// The Allies expect reserves when they have a reserve division.
void opeu_code_0e57_010e(Game& game, const uint8_t*, HostEvent) {
  ExpectReserves(game, 1);
}

// The Allied pool gains 500, up to 3000.
void opeu_code_0e57_0140(Game& game, const uint8_t*, HostEvent) {
  opeu_add_word(500, 3000, &Data(game).sides[1].bombers);
}

// Opens (show 1) the panel with the person's post, name and portrait, or closes it and waits for a key.
uint16_t opeu_noon_portrait(Game& game, const uint8_t*, HostEvent event, uint16_t person, uint16_t show) {
  auto& data{ Data(game) };
  if (show == 1) {
    Event(game, event, NOON_PANEL, 0);
    Event(game, event, NOON_UI_TEXT, 1, At(offsetof(DataSegment, noon_role_style)));
    auto const role{ data.role_names[(Record<Person>(data, person).role & Bits(PersonRole::ROLE_MASK)) - 1] };
    Event(game, event, NOON_FRAME, 3, 0x2c, 0x11a, static_cast<uint8_t>(opeu_string_length(game, person) + opeu_string_length(game, role) + 5));
    Event(game, event, NOON_UI_TEXT, 1, role);
    Event(game, event, NOON_PRINT, 2, At(offsetof(DataSegment, noon_name_format)), person);
    Event(game, event, NOON_PORTRAIT, 3, 0x2d, 0x13c, person);
    Event(game, event, NOON_UI_TEXT, 1, At(offsetof(DataSegment, noon_speech_style)));
    Event(game, event, NOON_POSITION, 2, 0x36, 0x144);
  } else {
    Event(game, event, NOON_UI_TEXT, 1, At(offsetof(DataSegment, noon_speech_end)));
    Event(game, event, NOON_WAIT, 1, 1);
  }
  return 1;
}

// The person speaks the text in the portrait panel.
void opeu_noon_speech(Game& game, const uint8_t* far_text, HostEvent event, uint16_t speech, uint16_t person) {
  if (opeu_noon_portrait(game, far_text, event, person, 1)) {
    Event(game, event, NOON_FEED, 1, speech);
    opeu_noon_portrait(game, far_text, event, person, 0);
  }
}

// The person template the code names speaks noon_speeches[line] when the audience hears it.
void opeu_noon_leader(Game& game, const uint8_t* far_text, HostEvent event, uint8_t code, uint8_t line) {
  auto const& data{ Data(game) };
  auto        audience{ static_cast<uint8_t>(code & AUDIENCE_MASK) };
  auto const  allied_solo{ data.side_control[0] == SIDE_COMPUTER && data.side_control[1] == SIDE_PLAYED };
  auto const  axis_solo{ data.side_control[0] == SIDE_PLAYED && data.side_control[1] == SIDE_COMPUTER };
  if ((audience == NOT_ALLIED_SOLO && !allied_solo) || (audience == NOT_AXIS_SOLO && !axis_solo)) audience = EVERYONE;
  if (audience != EVERYONE) return;
  ServiceLocals const locals{ SPEECH_BYTES };
  auto const          speech{ opeu_string_copy_far(game, locals.At(0), const_cast<uint8_t*>(far_text), data.noon_speeches[line].offset) };
  opeu_noon_speech(game, far_text, event, speech, At(PERSON_TEMPLATES_AT + sizeof(Person) * (code >> 2)));
}

// The day's noon events (at 12:00).
void opeu_turn_noon_events(Game& game, const uint8_t* far_text, HostEvent event, uint16_t code_segment) {
  ServiceLocals const locals{ NEWS_BYTES };
  auto&               data{ Data(game) };
  auto const          news{ locals.At(0) };
  Event(game, event, NOON_DAILY_CONTEXT, 1, data.current_side);
  for (auto const& noon : data.noon_events) {
    if (noon.date == END_OF_EVENTS) break;
    if (noon.date >> 4 != data.year || (noon.date & 0x0f) != data.month || noon.day != data.day) continue;
    if (noon.action.offset | noon.action.segment) RunAction(game, far_text, event, code_segment, noon.action);
    Event(game, event, NOON_ACTIVE_BEGIN, 0);
    opeu_string_copy_far(game, news, const_cast<uint8_t*>(far_text), noon.news.offset);
    opeu_string_append(game, news, At(offsetof(DataSegment, noon_news_end)));
    if (far_text[noon.news.offset]) Event(game, event, NOON_SOUND, 0);
    Event(game, event, NOON_MESSAGE, 2, news, opeu_find_side_record(game, data.current_side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF)));
    if (noon.leader != NO_SPEECH) opeu_noon_leader(game, far_text, event, noon.leader, noon.line);
  }
}

// The services of the noon events (NoonService).
uint16_t svc_noonevt(Game& game, uint16_t id, const uint16_t* args, uint16_t count) {
  switch (id) {
  case NOON_PANEL:         opeu_message_panel_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case NOON_UI_TEXT:       opeu_ui_text(game, (TextHost*)port_textout_host(), Arg(args, count, 0)); break;
  case NOON_FRAME:         opeu_context_frame(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case NOON_PRINT:         opeu_cursor_print(game, { args, count }); break;
  case NOON_PORTRAIT:      opeu_context_portrait(game, (const UiHost*)port_uiprim_host(), Arg(args, count, 0), Arg(args, count, 1), Arg(args, count, 2)); break;
  case NOON_POSITION:      opeu_army_position(game, (TextHost*)port_textout_host(), (int16_t)Arg(args, count, 0), (int16_t)Arg(args, count, 1)); break;
  case NOON_WAIT:          opeu_wait_confirm(game, (int16_t)Arg(args, count, 0)); break;
  case NOON_FEED:          opeu_text_feed(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0)); break;
  case NOON_DAILY_CONTEXT: opeu_daily_context(game, (uint8_t)Arg(args, count, 0)); break;
  case NOON_ACTIVE_BEGIN:  opeu_turn_active_begin(game, (const DialogHost*)port_dialogs_host()); break;
  case NOON_SOUND:         opeu_sound_cue10(game); break;
  case NOON_MESSAGE:       opeu_control_message(game, (const DialogHost*)port_dialogs_host(), Arg(args, count, 0), Arg(args, count, 1)); break;
  case NOON_FAR_ACTION:    port_call(game, (uint32_t)Arg(args, count, 1) * 16u + Arg(args, count, 0), nullptr, 0); break;
  default:                 port_unhandled("noonevt", id, count);
  }
  return 0;
}
