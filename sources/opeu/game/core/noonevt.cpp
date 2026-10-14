// The dated noon events: at 12:00 each event of the day shows its news, runs its action (blocking or allowing the
// Axis requests, raising or lowering a side's morale, readying reserves, adding to the Allied pool) and may have a
// leader speak.
#include "core/noonevt.hpp"
#include "common/dispatch.hpp"
#include "common/events.hpp"
#include "common/forces.hpp"
#include "common/memory.hpp"
#include "core/memory.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/strings.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <utility>

#include "common/game.hpp"
#include "core/context.hpp"
#include "core/dialogs.hpp"
#include "core/textout.hpp"
#include "core/uiprim.hpp"
#include "io/keyboard.hpp"
#include "io/soundcue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace {

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

// The sides whose morale or reserves an action changes.
constexpr uint8_t AXIS{ 0 };
constexpr uint8_t ALLIES{ 1 };

void SetRequests(Game& game, RequestKind kind, uint16_t enabled) {
  Data(game).special[0].enabled[std::to_underlying(kind)] = enabled;
}

// Each element of the side gains 30..49 morale, up to 99.
void RaiseMorale(Game& game, uint8_t side) {
  std::ranges::for_each(std::span{ Data(game).elements }.subspan(side * ELEMENTS_PER_SIDE, ELEMENTS_PER_SIDE),
                        [&game](Element& element) { element.morale = AddByte(element.morale, game.State().RandomBounded(20) + 30, 99); });
}

// Each element of the side loses 30..49 morale.
void LowerMorale(Game& game, uint8_t side) {
  std::ranges::for_each(std::span{ Data(game).elements }.subspan(side * ELEMENTS_PER_SIDE, ELEMENTS_PER_SIDE),
                        [&game](Element& element) { element.morale = SubByte(element.morale, game.State().RandomBounded(20) + 30); });
}

// A side with a reserve division expects its reserves.
void ExpectReserves(Game& game, uint8_t side) {
  auto&      data{ Data(game) };
  auto const groups{ std::views::iota(uint8_t{ 0 }, GROUPS_PER_SIDE) };
  if (std::ranges::any_of(groups, [&data, side](uint8_t group) { return (GroupOf(data, side, group).flags & Bits(GroupFlag::RESERVE)) != 0; }))
    data.side_flags[side] |= Bits(SideFlag::RESERVES_PENDING);
}

void NoAction(Game&) { }

void BlockAxisRegiments(Game& game) {
  SetRequests(game, RequestKind::REGIMENT, 0);
}

void BlockAxisSuppliesAndBattalions(Game& game) {
  SetRequests(game, RequestKind::SUPPLIES, 0);
  SetRequests(game, RequestKind::BATTALION, 0);
}

void BlockAxisBattalionsAndRegiments(Game& game) {
  SetRequests(game, RequestKind::BATTALION, 0);
  SetRequests(game, RequestKind::REGIMENT, 0);
}

void AllowAxisBattalionsAndRegiments(Game& game) {
  SetRequests(game, RequestKind::BATTALION, 1);
  SetRequests(game, RequestKind::REGIMENT, 1);
}

void RaiseAxisMorale(Game& game) {
  RaiseMorale(game, AXIS);
}

void LowerAxisMorale(Game& game) {
  LowerMorale(game, AXIS);
}

void LowerAlliedMorale(Game& game) {
  LowerMorale(game, ALLIES);
}

void AxisMoraleBlockade(Game& game) {
  LowerAxisMorale(game);
  BlockAxisSuppliesAndBattalions(game);
}

void AxisReserves(Game& game) {
  ExpectReserves(game, AXIS);
}

void AlliedReserves(Game& game) {
  ExpectReserves(game, ALLIES);
}

// The Allied pool gains 500, up to 3000.
void AlliedPool(Game& game) {
  Data(game).sides[ALLIES].bombers = AddWord(Data(game).sides[ALLIES].bombers, 500, 3000);
}

// The noon actions, by the enumerator the event records hold (NONE never runs).
using NoonRoutine           = auto (*)(Game&) -> void;
constexpr auto NOON_ACTIONS = DispatchRows<NoonAction, NoonRoutine>({
    { NoonAction::NOTHING,                             NoAction                        },
    { NoonAction::BLOCK_AXIS_REGIMENTS,                BlockAxisRegiments              },
    { NoonAction::BLOCK_AXIS_SUPPLIES_AND_BATTALIONS,  BlockAxisSuppliesAndBattalions  },
    { NoonAction::BLOCK_AXIS_BATTALIONS_AND_REGIMENTS, BlockAxisBattalionsAndRegiments },
    { NoonAction::ALLOW_AXIS_BATTALIONS_AND_REGIMENTS, AllowAxisBattalionsAndRegiments },
    { NoonAction::RAISE_AXIS_MORALE,                   RaiseAxisMorale                 },
    { NoonAction::LOWER_AXIS_MORALE,                   LowerAxisMorale                 },
    { NoonAction::LOWER_ALLIED_MORALE,                 LowerAlliedMorale               },
    { NoonAction::AXIS_MORALE_BLOCKADE,                AxisMoraleBlockade              },
    { NoonAction::AXIS_RESERVES,                       AxisReserves                    },
    { NoonAction::ALLIED_RESERVES,                     AlliedReserves                  },
    { NoonAction::ALLIED_POOL,                         AlliedPool                      },
});

constexpr auto At(std::size_t offset) -> uint16_t {
  return static_cast<uint16_t>(offset);
}

// Opens the panel with the person's post, name and portrait.
void OpenSpeakerPanel(Game& game, uint16_t person) {
  auto& data{ Data(game) };
  opeu_message_panel_begin(game);
  opeu_ui_text(game, At(offsetof(DataSegment, noon_role_style)));
  auto const role{ data.role_names[(Record<Person>(data, person).role & Bits(PersonRole::ROLE_MASK)) - 1] };
  opeu_context_frame(game, 0x2c, 0x11a, static_cast<uint8_t>(opeu_string_length(game, person) + opeu_string_length(game, role) + 5));
  opeu_ui_text(game, role);
  opeu_cursor_print(game, std::array<FormatArgument, 2>{ At(offsetof(DataSegment, noon_name_format)), person });
  opeu_context_portrait(game, 0x2d, 0x13c, person);
  opeu_ui_text(game, At(offsetof(DataSegment, noon_speech_style)));
  opeu_army_position(game, 0x36, 0x144);
}

// Closes the speaker's panel and waits for a key.
void CloseSpeakerPanel(Game& game) {
  opeu_ui_text(game, At(offsetof(DataSegment, noon_speech_end)));
  opeu_wait_confirm(game);
}

// The person speaks the text in the portrait panel.
void Speak(Game& game, Text speech, uint16_t person) {
  OpenSpeakerPanel(game, person);
  opeu_text_feed(game, speech);
  CloseSpeakerPanel(game);
}

// Whether the audience the leader code names hears the speech in the current game: everyone always does; the
// audiences that exclude one side played alone against the computer hear it in every other game.
auto Heard(DataSegment const& data, uint8_t code) -> bool {
  auto const audience{ static_cast<uint8_t>(code & AUDIENCE_MASK) };
  auto const allied_solo{ data.side_control[0] == SIDE_COMPUTER && data.side_control[1] == SIDE_PLAYED };
  auto const axis_solo{ data.side_control[0] == SIDE_PLAYED && data.side_control[1] == SIDE_COMPUTER };
  return audience == EVERYONE || (audience == NOT_ALLIED_SOLO && !allied_solo) || (audience == NOT_AXIS_SOLO && !axis_solo);
}

// The person template the event's leader code names speaks noon_speeches[line] when the audience hears it.
void LeaderSpeaks(Game& game, std::span<std::uint8_t const> far_text, NoonEvent const& noon) {
  auto const& data{ Data(game) };
  if (!Heard(data, noon.leader)) return;
  TextBuffer<SPEECH_BYTES> speech;
  opeu_string_copy_far(game, speech, far_text.subspan(data.noon_speeches[noon.line].offset));
  Speak(game, speech, At(PERSON_TEMPLATES_AT + sizeof(Person) * (noon.leader >> 2)));
}

} // namespace

// The day's noon events (at 12:00).
void opeu_turn_noon_events(Game& game, std::span<std::uint8_t const> far_text) {
  TextBuffer<NEWS_BYTES> news;
  auto&                  data{ Data(game) };
  auto const             listed{ [](NoonEvent const& noon) { return noon.date != END_OF_EVENTS; } };
  auto const             today{ [&data](NoonEvent const& noon) { return noon.date >> 4 == data.year && (noon.date & 0x0f) == data.month && noon.day == data.day; } };
  opeu_daily_context(game, data.current_side);
  for (auto const& noon : data.noon_events | std::views::take_while(listed) | std::views::filter(today)) {
    if (noon.action != NoonAction::NONE) Dispatch(NOON_ACTIONS, noon.action, game);
    opeu_turn_active_begin(game);
    opeu_string_copy_far(game, news, far_text.subspan(noon.news.offset));
    opeu_string_append(game, news, At(offsetof(DataSegment, noon_news_end)));
    if (far_text[noon.news.offset]) opeu_sound_cue10(game);
    opeu_control_message(game, news, opeu_find_side_record(game, data.current_side, std::to_underlying(StaffRole::INTELLIGENCE_CHIEF)));
    if (noon.leader != NO_SPEECH) LeaderSpeaks(game, far_text, noon);
  }
}
