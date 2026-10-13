// Hourly wound recovery of both sides' generals: a wounded general recovers or loses health, and at no health a
// deputy holds on, a role-1 general becomes a deputy and anyone else dies and is replaced by a new deputy. Every
// general then heals a little, up to his health limit.
#include "core/evwound.hpp"
#include "common/events.hpp"
#include "core/counter.hpp"
#include "core/deputy.hpp"
#include "core/numeric.hpp"
#include "core/people.hpp"
#include "core/personfl.hpp"
#include "core/strings.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "core/dialogs.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

#include <array>
#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint16_t DEATH_PREFIX{ offsetof(DataSegment, death_prefix) };
constexpr std::uint16_t DEATH_SUFFIX{ offsetof(DataSegment, death_suffix) };
constexpr std::uint16_t DEATH_SHOW{ 300 };
constexpr std::uint16_t DEATH_CONTEXT{ 0x10 };
constexpr std::uint8_t  ADVISER{ 3 };      // the side record kind of the adviser
constexpr std::uint8_t  GENERAL_ROLE{ 1 }; // the role that becomes a deputy instead of dying

// The death notice in the dying general's reserve (the message service shows it): "Commander " + name + suffix, and the side right after it.
// A name of seven or more characters runs the text into the side byte, and the second status test then reads the
// overwritten side.
struct DeathNotice {
  std::array<char, 50> text;
  std::uint8_t         side;
};
constexpr std::uint16_t NOTICE_BYTES{ 0x34 };
} // namespace

// The presentation (Presenter::DEATH): a pause.
void opeu_death_present(Game& game) {
  opeu_delay_units(game, 30);
}

// Replaces the dead general with a new deputy; a side the player watches is told by its adviser.
void opeu_commander_death(Game& game, uint16_t person) {
  ServiceLocals const locals{ NOTICE_BYTES };
  auto&               data{ Data(game) };
  auto const          at{ locals.At(0) };
  auto&               notice{ Record<DeathNotice>(data, at) };
  notice.side = opeu_person_lacks_side0_flag(game, person);
  if (data.side_status[notice.side] == SIDE_PLAYED) {
    opeu_string_copy(game, at, DEATH_PREFIX);
    opeu_string_append(game, at, person);
    opeu_string_append(game, at, DEATH_SUFFIX);
  }
  opeu_initialize_deputy(game, person);
  if (data.side_status[notice.side] == SIDE_PLAYED) {
    opeu_set_turn_context(game, DEATH_CONTEXT);
    opeu_finish_side_initialization(game, 0);
    opeu_daily_show(game, DEATH_SHOW, notice.side, Presenter::DEATH);
    opeu_control_message(game, at, opeu_find_side_record(game, notice.side, ADVISER));
  }
}

// A wounded general recovers with chance 1 in (level * 10 + 24), the level being the delivery level for a side the
// player commands and 2 otherwise; else he loses 10 - limit / 10 health. Healing: 1 for a general assigned to a
// division, 3 while his record counter runs, else 2.
void opeu_wound_event(Game& game) {
  auto& data{ Data(game) };
  for (std::uint8_t side{ 0 }; side < SIDE_COUNT; ++side) {
    std::uint16_t const level{ DeliveryLevel(data, side) };
    for (std::uint8_t i{ 0 }; i < PEOPLE_PER_SIDE; ++i) {
      auto& person{ data.people[side * PEOPLE_PER_SIDE + i] };
      if (person.status == PERSON_VACANT) continue;
      if (person.role & Bits(PersonRole::POOR_HEALTH)) {
        if (game.State().RandomBounded(static_cast<std::uint16_t>(level * 10 + 24)) == 2)
          person.role &= static_cast<std::uint8_t>(~Bits(PersonRole::POOR_HEALTH));
        else
          opeu_sub_byte(10 - person.max_health / 10, &person.health);
        if (person.health == 0) {
          if (person.role & Bits(PersonRole::DEPUTY))
            person.health = 1;
          else if ((person.role & Bits(PersonRole::ROLE_MASK)) == GENERAL_ROLE)
            person.role |= Bits(PersonRole::DEPUTY);
          else
            opeu_commander_death(game, Near(data, person));
        }
      }
      std::uint8_t amount{ 2 };
      if (person.assignment & Bits(Assignment::DIVISION))
        amount = 1;
      else if (opeu_get_record_counter(person))
        amount = 3;
      opeu_add_byte(amount, person.max_health, &person.health);
    }
  }
}
