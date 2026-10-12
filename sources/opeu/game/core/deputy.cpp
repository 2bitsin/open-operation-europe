// Deputies: a new deputy's identity (portrait) and starting ratings.
#include "core/deputy.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/random.hpp"
#include "core/personfl.hpp"
#include "core/strings.hpp"
#include "port.hpp"

#include <cstddef>

using namespace opeu::game;

namespace {

constexpr std::uint8_t  DEPUTY_RATING{ 30 };
constexpr std::uint16_t DEPUTY_NAME{ offsetof(DataSegment, deputy_name) };

// The portraits a side's deputies may wear: first and last.
struct IdentityRange {
  std::uint8_t first;
  std::uint8_t last;
};

auto Range(DataSegment const& data, std::uint8_t side) -> IdentityRange {
  return { data.deputy_identity_ranges[2 * side], data.deputy_identity_ranges[2 * side + 1] };
}
} // namespace

// The next free portrait above the side's deputies, or a random one of the side's range when the range is used up.
uint8_t opeu_choose_deputy_identity(uint8_t ds[65536], uint8_t side) {
  auto&        data{ Data(ds) };
  auto const   range{ Range(data, side) };
  std::uint8_t identity{ range.first };
  for (std::uint8_t i{ 0 }; i < PEOPLE_PER_SIDE; ++i) {
    auto const& person{ data.people[PEOPLE_PER_SIDE * side + i] };
    if (person.status != PERSON_VACANT && (person.role & Bits(PersonRole::DEPUTY)) && person.portrait > identity)
      identity = person.portrait;
  }
  if (range.last <= identity)
    identity = static_cast<std::uint8_t>(RandomBelow(ds, static_cast<std::uint16_t>(range.last - range.first + 1))
                                         + range.first);
  else
    ++identity;
  return identity;
}

// Makes the general a deputy: the deputy's name, every rating 30, no experience, a portrait of the general's side.
void opeu_initialize_deputy(uint8_t ds[65536], uint16_t person) {
  auto& data{ Data(ds) };
  auto& deputy{ Record<Person>(data, person) };
  deputy.role |= Bits(PersonRole::DEPUTY);
  opeu_string_copy(ds, person, DEPUTY_NAME);
  deputy.leadership = DEPUTY_RATING;
  deputy.charm      = DEPUTY_RATING;
  deputy.bravery    = DEPUTY_RATING;
  deputy.byte_17    = DEPUTY_RATING;
  deputy.health     = DEPUTY_RATING;
  deputy.infantry   = DEPUTY_RATING;
  deputy.artillery  = DEPUTY_RATING;
  deputy.armor      = DEPUTY_RATING;
  deputy.experience = 0;
  deputy.portrait   = opeu_choose_deputy_identity(ds, opeu_person_lacks_side0_flag(ds, person));
}

auto port_routine_opeu_initialize_deputy(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  opeu_initialize_deputy(ds, Arg(args, count, 0));
  return 0;
}
