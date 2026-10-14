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
#include "core/dialogs.hpp"
#include "io/sound.hpp"
#include "io/waitkey.hpp"
#include "port.hpp"
#include "services.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>

using namespace opeu::game;

namespace {

constexpr std::uint16_t DEATH_PREFIX{ offsetof(DataSegment, death_prefix) };
constexpr std::uint16_t DEATH_SUFFIX{ offsetof(DataSegment, death_suffix) };
constexpr std::uint16_t DEATH_SHOW{ 300 };
constexpr std::uint16_t DEATH_CONTEXT{ 0x10 };
constexpr std::uint8_t  ADVISER{ 3 };      // the side record kind of the adviser
constexpr std::uint8_t  GENERAL_ROLE{ 1 }; // the role that becomes a deputy instead of dying

// The status of a side as the notice's tests read it: the word at side_status[side], past the array for a side byte
// a long name overwrote.
auto SideStatus(Game const& game, std::uint8_t side) -> std::uint16_t {
  return Word(game, static_cast<std::uint16_t>(offsetof(DataSegment, side_status) + (std::size_t{ side } * 2)));
}

// A wounded general's hour: he recovers with chance 1 in (level * 10 + 24), else he loses 10 - limit / 10 health; at
// no health a deputy holds on, a role-1 general becomes a deputy and anyone else dies.
auto Wounded(Game& game, Person& person, std::uint16_t level) -> void {
  if (game.State().RandomBounded(static_cast<std::uint16_t>((level * 10) + 24)) == 2)
    person.role &= static_cast<std::uint8_t>(~Bits(PersonRole::POOR_HEALTH));
  else
    person.health = SubByte(person.health, 10 - (person.max_health / 10));
  if (person.health != 0) return;
  if ((person.role & Bits(PersonRole::DEPUTY)) != 0)
    person.health = 1;
  else if ((person.role & Bits(PersonRole::ROLE_MASK)) == GENERAL_ROLE)
    person.role |= Bits(PersonRole::DEPUTY);
  else
    opeu_commander_death(game, Near(Data(game), person));
}

// The hour's healing: 1 for a general assigned to a division, 3 while his record counter runs, else 2.
auto Healing(Person const& person) -> std::uint8_t {
  if ((person.assignment & Bits(Assignment::DIVISION)) != 0) return 1;
  return opeu_get_record_counter(person) != 0 ? 3 : 2;
}
} // namespace

// The presentation (Presenter::DEATH): a pause.
void opeu_death_present(Game& game) {
  opeu_delay_units(game, 30);
}

auto opeu::game::detail::evwound::ComposeDeathNotice(Game& game, std::uint16_t person) -> DeathNotice {
  DeathNotice notice{ opeu_person_lacks_side0_flag(game, person) };
  if (SideStatus(game, notice.Side()) != SIDE_PLAYED) return notice;
  opeu_string_copy(game, notice.Message(), DEATH_PREFIX);
  opeu_string_append(game, notice.Message(), person);
  opeu_string_append(game, notice.Message(), DEATH_SUFFIX);
  return notice;
}

// Replaces the dead general with a new deputy; a side the player watches is told by its adviser.
void opeu_commander_death(Game& game, uint16_t person) {
  auto notice{ ComposeDeathNotice(game, person) };
  opeu_initialize_deputy(game, person);
  std::uint8_t const side{ notice.Side() };
  if (SideStatus(game, side) == SIDE_PLAYED) {
    opeu_set_turn_context(game, DEATH_CONTEXT);
    opeu_finish_side_initialization(game, 0);
    opeu_daily_show(game, DEATH_SHOW, side, Presenter::DEATH);
    opeu_control_message(game, notice.Message(), opeu_find_side_record(game, side, ADVISER));
  }
}

// Every present general of both sides: the wounded recover or worsen, then all heal a little up to their limit.
void opeu_wound_event(Game& game) {
  auto& data{ Data(game) };
  std::ranges::for_each(std::views::iota(std::uint8_t{ 0 }, std::uint8_t{ SIDE_COUNT }), [&](std::uint8_t side) {
    auto const level{ DeliveryLevel(data, side) };
    for (auto& person : std::span{ data.people }.subspan(std::size_t{ side } * PEOPLE_PER_SIDE, PEOPLE_PER_SIDE)) {
      if (person.status == PERSON_VACANT) continue;
      if ((person.role & Bits(PersonRole::POOR_HEALTH)) != 0) Wounded(game, person, level);
      person.health = AddByte(person.health, Healing(person), person.max_health);
    }
  });
}
