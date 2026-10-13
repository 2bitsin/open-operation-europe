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

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t BUFFER{ offsetof(DataSegment, event_buffer) };
constexpr std::uint16_t NAME_FORMAT{ offsetof(DataSegment, spy_name_format) };
constexpr std::uint16_t LAUGH{ offsetof(DataSegment, spy_laugh_text) };
constexpr std::uint16_t MOVE{ offsetof(DataSegment, spy_move_text) };
constexpr std::uint16_t BOLD_OFF{ offsetof(DataSegment, spy_bold_off) };
constexpr std::uint16_t HUMPH{ offsetof(DataSegment, spy_humph_text) };
constexpr std::uint16_t PORTRAIT_LOW{ 0x7500 }; // her portrait's position in the portrait file
constexpr std::uint16_t PORTRAIT_HIGH{ 3 };
constexpr std::uint16_t SPY_CONTEXT{ 0x11 };
constexpr std::uint16_t GREETING_SHOW{ 0xba };
constexpr std::uint16_t ENDING_SHOW{ 0xca };
constexpr Scenario      SPY_SCENARIO{ Scenario::BULGE };
constexpr std::uint8_t  COMMANDER{ 1 };
constexpr std::uint8_t  ADVISER{ 2 };
constexpr std::uint8_t  OFFER_HOUR{ 15 };
constexpr std::uint8_t  APPROACH_HOUR{ 20 };
constexpr std::uint8_t  FIT{ 70 };
constexpr std::uint8_t  EXPERIENCED{ 50 };

auto Lookup(Game& game, std::uint16_t id) -> std::uint16_t {
  return opeu_message_lookup(game, static_cast<int16_t>(id));
}
auto Commander(Game& game) -> std::uint16_t {
  return opeu_find_side_record(game, 0, COMMANDER);
}
} // namespace

// Whether side 0's commander is fit for the visit, the weather calm (clear or 1) and his unit safe.
uint16_t opeu_spy_commander_ready(Game& game) {
  auto&       data{ Data(game) };
  auto const  near{ Commander(game) };
  auto const  unit{ opeu_person_selected_unit(game, near) };
  auto const& commander{ Record<Person>(data, near) };
  return (data.weather == 0 || data.weather == 1) && commander.charm >= FIT && commander.health >= FIT && commander.experience >= EXPERIENCED && opeu_visit_unit_safe(game, Record<Unit>(data, unit))
             ? 1
             : 0;
}

// The message panel with her name and, when `show`, her portrait; then the text.
void opeu_spy_portrait_text(Game& game, uint16_t text, uint16_t show) {
  auto const& data{ Data(game) };
  opeu_message_panel_begin(game);
  opeu_context_frame(game, 44, 282, 15);
  opeu_cursor_print(game, std::array<std::uint16_t, 1>{ static_cast<std::uint16_t>(Lookup(game, 0x2b)) });
  if (show) {
    opeu_dos_seek(static_cast<int16_t>(data.portrait_handle),
                  static_cast<int32_t>(static_cast<std::uint32_t>(static_cast<std::uint16_t>(PORTRAIT_LOW)) | static_cast<std::uint32_t>(static_cast<std::uint16_t>(PORTRAIT_HIGH)) << 16),
                  static_cast<int16_t>(0));
    opeu_image_read_draw(game, 45, 316, 8, 80, 0, static_cast<int16_t>(data.portrait_handle));
  }
  opeu_army_position(game, static_cast<int16_t>(54), static_cast<int16_t>(324));
  opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(BUFFER), static_cast<std::uint16_t>(NAME_FORMAT), static_cast<std::uint16_t>(text) });
  opeu_text_feed(game, BUFFER);
}

// The greeting scene (messages 44..51); the commander's charm rises by 1.
void opeu_spy_present_a(Game& game) {
  opeu_set_turn_context(game, static_cast<std::uint8_t>(SPY_CONTEXT));
  auto const near{ Commander(game) };
  for (std::uint16_t id{ 0x2c }; id <= 0x33; ++id) {
    opeu_request_format(game, std::array<std::uint16_t, 3>{ static_cast<std::uint16_t>(BUFFER), static_cast<std::uint16_t>(Lookup(game, id)), static_cast<std::uint16_t>(near) });
    opeu_spy_portrait_text(game, BUFFER, 1);
  }
  opeu_finish_side_initialization(game, static_cast<std::uint8_t>(0));
  opeu_add_byte(1, RATING_LIMIT, &Record<Person>(Data(game), near).charm);
}

void opeu_spy_show_a(Game& game) {
  opeu_daily_show(game, GREETING_SHOW, 0, Presenter::SPY_GREETING);
}

// The accepted ending: experience up by 1, the health limit down by 1 and health capped at it.
void opeu_spy_present_b(Game& game) {
  opeu_request_format(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(BUFFER), static_cast<std::uint16_t>(LAUGH) });
  opeu_spy_portrait_text(game, BUFFER, 1);
  auto& commander{ Record<Person>(Data(game), Commander(game)) };
  opeu_add_byte(1, RATING_LIMIT, &commander.experience);
  opeu_sub_byte(1, &commander.max_health);
  commander.health = static_cast<std::uint8_t>(opeu_min16(commander.health, commander.max_health));
}

// The approach scene and its question.
void opeu_spy_approach(Game& game) {
  auto const& data{ Data(game) };
  opeu_set_turn_context(game, static_cast<std::uint8_t>(SPY_CONTEXT));
  opeu_request_format(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(BUFFER), static_cast<std::uint16_t>(Lookup(game, 0x6b)) });
  opeu_spy_portrait_text(game, BUFFER, 1);
  opeu_request_format(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(BUFFER), static_cast<std::uint16_t>(Lookup(game, 0x6c)) });
  opeu_spy_portrait_text(game, BUFFER, 1);
  opeu_message_panel_begin(game);
  if (data.restart_flag == 1) {
    opeu_text_feed(game, MOVE);
    auto const accepted{ static_cast<uint16_t>(opeu_yes_no(game)) };
    opeu_ui_text(game, BOLD_OFF);
    if (accepted)
      opeu_daily_show(game, ENDING_SHOW, 0, Presenter::SPY_ENDING);
    else {
      opeu_request_format(game, std::array<std::uint16_t, 2>{ static_cast<std::uint16_t>(BUFFER), static_cast<std::uint16_t>(HUMPH) });
      opeu_spy_portrait_text(game, BUFFER, 1);
    }
  }
  opeu_finish_side_initialization(game, static_cast<std::uint8_t>(0));
}

// The hourly check (SideFlag::SPY_SENT: the offer was taken, SPY_DONE: the event is over for the day).
void opeu_spy_event(Game& game) {
  auto& data{ Data(game) };
  if (CurrentScenario(data) != SPY_SCENARIO || data.side_control[0] != SIDE_PLAYED) return;
  auto const adviser{ opeu_find_side_record(game, 0, ADVISER) };
  auto&      flags{ data.side_flags[0] };
  auto const sent{ Bits(SideFlag::SPY_SENT) };
  auto const over{ static_cast<std::uint8_t>(Bits(SideFlag::SPY_SENT) | Bits(SideFlag::SPY_DONE)) };
  if (data.hour == OFFER_HOUR && !(flags & sent) && opeu_spy_commander_ready(game)) {
    opeu_control_message(game, Lookup(game, 0x38), adviser);
    if (opeu_confirm_message(game, Lookup(game, 0x39), adviser)) {
      opeu_spy_show_a(game);
      flags |= sent;
    } else
      flags |= over;
  }
  if (data.hour == APPROACH_HOUR && (flags & over) == sent && opeu_spy_commander_ready(game)) {
    if (opeu_confirm_message(game, Lookup(game, 0x3a), adviser)) opeu_spy_approach(game);
    flags |= Bits(SideFlag::SPY_DONE);
  }
}
