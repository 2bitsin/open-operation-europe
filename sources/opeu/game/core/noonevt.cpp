// The dated noon events: at 12:00 each event of the day shows its news, runs its action (blocking or allowing the
// Axis requests, raising or lowering a side's morale, readying reserves, adding to the Allied pool) and may have a
// leader speak.
#include "core/noonevt.hpp"
#include "common/dispatch.hpp"
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

#include <array>
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
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

// The noon actions, by the enumerator the event records hold (NONE never runs).
using NoonRoutine           = auto (*)(Game&, std::uint8_t const*) -> void;
constexpr auto NOON_ACTIONS = DispatchRows<NoonAction, NoonRoutine>({
    { NoonAction::NOTHING,                             opeu_noon_no_action            },
    { NoonAction::BLOCK_AXIS_REGIMENTS,                opeu_noon_block_axis_regts     },
    { NoonAction::BLOCK_AXIS_SUPPLIES_AND_BATTALIONS,  opeu_noon_block_axis_sup_bns   },
    { NoonAction::BLOCK_AXIS_BATTALIONS_AND_REGIMENTS, opeu_noon_block_axis_bns_regts },
    { NoonAction::ALLOW_AXIS_BATTALIONS_AND_REGIMENTS, opeu_noon_allow_axis_bns_regts },
    { NoonAction::RAISE_AXIS_MORALE,                   opeu_noon_raise_axis_morale    },
    { NoonAction::LOWER_AXIS_MORALE,                   opeu_noon_lower_axis_morale    },
    { NoonAction::LOWER_ALLIED_MORALE,                 opeu_noon_lower_allied_morale  },
    { NoonAction::AXIS_MORALE_BLOCKADE,                opeu_noon_axis_morale_blockade },
    { NoonAction::AXIS_RESERVES,                       opeu_code_0e57_00dc            },
    { NoonAction::ALLIED_RESERVES,                     opeu_code_0e57_010e            },
    { NoonAction::ALLIED_POOL,                         opeu_code_0e57_0140            },
});

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

constexpr auto At(std::size_t offset) -> uint16_t {
  return static_cast<uint16_t>(offset);
}

} // namespace

void opeu_noon_no_action(Game&, const uint8_t*) { }

void opeu_noon_block_axis_regts(Game& game, const uint8_t*) {
  SetRequests(game, RequestKind::REGIMENT, 0);
}

void opeu_noon_block_axis_sup_bns(Game& game, const uint8_t*) {
  SetRequests(game, RequestKind::SUPPLIES, 0);
  SetRequests(game, RequestKind::BATTALION, 0);
}

void opeu_noon_block_axis_bns_regts(Game& game, const uint8_t*) {
  SetRequests(game, RequestKind::BATTALION, 0);
  SetRequests(game, RequestKind::REGIMENT, 0);
}

void opeu_noon_allow_axis_bns_regts(Game& game, const uint8_t*) {
  SetRequests(game, RequestKind::BATTALION, 1);
  SetRequests(game, RequestKind::REGIMENT, 1);
}

void opeu_noon_raise_axis_morale(Game& game, const uint8_t*) {
  ShiftMorale(game, 0, true);
}

void opeu_noon_lower_axis_morale(Game& game, const uint8_t*) {
  ShiftMorale(game, 0, false);
}

void opeu_noon_lower_allied_morale(Game& game, const uint8_t*) {
  ShiftMorale(game, 1, false);
}

void opeu_noon_axis_morale_blockade(Game& game, const uint8_t* far_text) {
  opeu_noon_lower_axis_morale(game, far_text);
  opeu_noon_block_axis_sup_bns(game, far_text);
}

// The Axis expects reserves when it has a reserve division.
void opeu_code_0e57_00dc(Game& game, const uint8_t*) {
  ExpectReserves(game, 0);
}

// The Allies expect reserves when they have a reserve division.
void opeu_code_0e57_010e(Game& game, const uint8_t*) {
  ExpectReserves(game, 1);
}

// The Allied pool gains 500, up to 3000.
void opeu_code_0e57_0140(Game& game, const uint8_t*) {
  opeu_add_word(500, 3000, &Data(game).sides[1].bombers);
}

// Opens (show 1) the panel with the person's post, name and portrait, or closes it and waits for a key.
uint16_t opeu_noon_portrait(Game& game, const uint8_t*, uint16_t person, uint16_t show) {
  auto& data{ Data(game) };
  if (show == 1) {
    opeu_message_panel_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), At(offsetof(DataSegment, noon_role_style)));
    auto const role{ data.role_names[(Record<Person>(data, person).role & Bits(PersonRole::ROLE_MASK)) - 1] };
    opeu_context_frame(game, static_cast<const UiHost*>(port_uiprim_host()), 0x2c, 0x11a, static_cast<uint8_t>(opeu_string_length(game, person) + opeu_string_length(game, role) + 5));
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), role);
    opeu_cursor_print(game, std::array<uint16_t, 2>{ At(offsetof(DataSegment, noon_name_format)), person });
    opeu_context_portrait(game, static_cast<const UiHost*>(port_uiprim_host()), 0x2d, 0x13c, person);
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), At(offsetof(DataSegment, noon_speech_style)));
    opeu_army_position(game, static_cast<TextHost*>(port_textout_host()), 0x36, 0x144);
  } else {
    opeu_ui_text(game, static_cast<TextHost*>(port_textout_host()), At(offsetof(DataSegment, noon_speech_end)));
    opeu_wait_confirm(game, 1);
  }
  return 1;
}

// The person speaks the text in the portrait panel.
void opeu_noon_speech(Game& game, const uint8_t* far_text, uint16_t speech, uint16_t person) {
  if (opeu_noon_portrait(game, far_text, person, 1)) {
    opeu_text_feed(game, static_cast<const DialogHost*>(port_dialogs_host()), speech);
    opeu_noon_portrait(game, far_text, person, 0);
  }
}

// The person template the code names speaks noon_speeches[line] when the audience hears it.
void opeu_noon_leader(Game& game, const uint8_t* far_text, uint8_t code, uint8_t line) {
  auto const& data{ Data(game) };
  auto        audience{ static_cast<uint8_t>(code & AUDIENCE_MASK) };
  auto const  allied_solo{ data.side_control[0] == SIDE_COMPUTER && data.side_control[1] == SIDE_PLAYED };
  auto const  axis_solo{ data.side_control[0] == SIDE_PLAYED && data.side_control[1] == SIDE_COMPUTER };
  if ((audience == NOT_ALLIED_SOLO && !allied_solo) || (audience == NOT_AXIS_SOLO && !axis_solo)) audience = EVERYONE;
  if (audience != EVERYONE) return;
  ServiceLocals const locals{ SPEECH_BYTES };
  auto const          speech{ opeu_string_copy_far(game, locals.At(0), const_cast<uint8_t*>(far_text), data.noon_speeches[line].offset) };
  opeu_noon_speech(game, far_text, speech, At(PERSON_TEMPLATES_AT + sizeof(Person) * (code >> 2)));
}

// The day's noon events (at 12:00).
void opeu_turn_noon_events(Game& game, const uint8_t* far_text) {
  ServiceLocals const locals{ NEWS_BYTES };
  auto&               data{ Data(game) };
  auto const          news{ locals.At(0) };
  opeu_daily_context(game, data.current_side);
  for (auto const& noon : data.noon_events) {
    if (noon.date == END_OF_EVENTS) break;
    if (noon.date >> 4 != data.year || (noon.date & 0x0f) != data.month || noon.day != data.day) continue;
    if (noon.action != NoonAction::NONE) Dispatch(NOON_ACTIONS, noon.action, game, far_text);
    opeu_turn_active_begin(game, static_cast<const DialogHost*>(port_dialogs_host()));
    opeu_string_copy_far(game, news, const_cast<uint8_t*>(far_text), noon.news.offset);
    opeu_string_append(game, news, At(offsetof(DataSegment, noon_news_end)));
    if (far_text[noon.news.offset]) opeu_sound_cue10(game);
    opeu_control_message(game, static_cast<const DialogHost*>(port_dialogs_host()), news, opeu_find_side_record(game, data.current_side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF)));
    if (noon.leader != NO_SPEECH) opeu_noon_leader(game, far_text, noon.leader, noon.line);
  }
}
