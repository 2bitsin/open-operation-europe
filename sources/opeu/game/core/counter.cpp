// The countdown a general keeps in the high five bits of Person::assignment.
#include "core/counter.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"

using namespace opeu::game;

namespace {

constexpr auto COUNTER_SHIFT{ 3 };
}

// Sets the countdown; the low bits of the byte are cleared.
void opeu_set_record_counter(Person& general, uint8_t count) {
  general.assignment = static_cast<std::uint8_t>(count << COUNTER_SHIFT);
}

uint8_t opeu_get_record_counter(Person const& general) {
  return general.assignment >> COUNTER_SHIFT;
}
