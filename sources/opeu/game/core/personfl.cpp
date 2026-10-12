// A general's nation, staff role and division assignment, and the name of a nation mask.
#include "types.hpp"
#include "core/personfl.hpp"
#include "data-segment.hpp"

using namespace opeu::game;

// The name of the lowest nation bit set in mask (one of six), 0 when none is.
uint16_t opeu_first_mask_name(const uint8_t ds[65536], uint8_t mask) {
  auto const& data{ Data(ds) };
  for (uint16_t a = 0; a < 6; ++a, mask >>= 1)
    if (mask & 1)
      return data.mask_names[a];
  return 0;
}

// 1 when the general does not serve side 0's nations.
uint8_t opeu_person_lacks_side0_flag(const uint8_t ds[65536], uint16_t person) {
  auto const& data{ Data(ds) };
  if (data.side_masks[0] & Record<Person>(data, person).status)
    return 0;
  return 1;
}

void opeu_person_set_role(uint8_t ds[65536], uint16_t person, uint8_t value) {
  auto& record{ Record<Person>(Data(ds), person) };
  record.role = (record.role & ~Bits(PersonRole::ROLE_MASK)) | value;
}

// Assigns the general to division value (0..3) of his side.
void opeu_person_set_group(uint8_t ds[65536], uint16_t person, uint8_t value) {
  Record<Person>(Data(ds), person).assignment =
      value | Bits(Assignment::DIVISION);
}
