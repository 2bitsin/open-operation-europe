// A general's nation, staff role and division assignment, and the name of a nation mask.
#include "core/personfl.hpp"
#include "common/game.hpp"
#include "data-segment.hpp"
#include "types.hpp"

using namespace opeu::game;

// The name of the lowest nation bit set in mask (one of six), 0 when none is.
uint16_t opeu_first_mask_name(Game const& game, uint8_t mask) {
  auto const& data{ Data(game) };
  for (uint16_t a = 0; a < 6; ++a, mask >>= 1)
    if (mask & 1) return data.mask_names[a];
  return 0;
}

// 1 when the general does not serve side 0's nations.
uint8_t opeu_person_lacks_side0_flag(Game const& game, uint16_t person) {
  auto const& data{ Data(game) };
  if (data.side_masks[0] & Record<Person>(data, person).status) return 0;
  return 1;
}

void opeu_person_set_role(Game& game, uint16_t person, uint8_t value) {
  auto& record{ Record<Person>(Data(game), person) };
  record.role = (record.role & ~Bits(PersonRole::ROLE_MASK)) | value;
}

// Assigns the general to division value (0..3) of his side.
void opeu_person_set_group(Game& game, uint16_t person, uint8_t value) {
  Record<Person>(Data(game), person).assignment = value | Bits(Assignment::DIVISION);
}
