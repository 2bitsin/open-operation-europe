// The first active, unassigned person of a side who is not a deputy and has no counter running.
#include "core/freepers.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"

using namespace opeu::game;

namespace {
constexpr std::uint8_t ABSENT{ 255 };      // Person::status of an empty slot
constexpr std::uint8_t BUSY_ROLES{ 0x87 }; // role bits of a person already holding a post
constexpr std::uint8_t ASSIGNED{ 0x04 };   // Person::assignment bit of an assigned person
constexpr unsigned     COUNTER_SHIFT{ 3 }; // Person::assignment keeps the counter in its high five bits
} // namespace

uint16_t opeu_find_free_person(const uint8_t ds[65536], uint8_t side) {
  auto const& data{ Data(ds) };
  for (unsigned i = 0; i < PEOPLE_PER_SIDE; ++i) {
    auto const& person{ data.people[side * PEOPLE_PER_SIDE + i] };
    if (person.status != ABSENT && !(person.role & BUSY_ROLES) && !(person.assignment & ASSIGNED) && !(person.assignment >> COUNTER_SHIFT)) return Near(data, person);
  }
  return 0;
}
