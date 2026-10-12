// The countdown a general keeps in the high five bits of Person::assignment; record is the general's record.
#include "types.hpp"
#include "core/counter.hpp"

#include "common/data-segment.hpp"

using namespace opeu::game;

namespace {

constexpr auto COUNTER_SHIFT{ 3 };
}

// Sets the countdown; the low bits of the byte are cleared.
void opeu_set_record_counter(uint8_t* record, uint8_t count) {
  reinterpret_cast<Person*>(record)->assignment =
      static_cast<std::uint8_t>(count << COUNTER_SHIFT);
}

uint8_t opeu_get_record_counter(uint8_t* record) {
  return reinterpret_cast<Person const*>(record)->assignment >> COUNTER_SHIFT;
}
