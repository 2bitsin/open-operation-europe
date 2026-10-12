// The unit a general currently has selected.
#include "types.hpp"
#include "core/perssel.hpp"
#include "core/personfl.hpp"
#include "core/selunit.hpp"
#include "data-segment.hpp"

using namespace opeu::game;

// A staff officer looks at his side's selected group, a division commander at his division; 0 for anyone else or
// when nothing is selected.
uint16_t opeu_person_selected_unit(const uint8_t ds[65536], uint16_t person) {
  auto const&   data{ Data(ds) };
  auto const&   record{ Record<Person>(data, person) };
  uint16_t      unit{ 0 };
  uint8_t const side{ opeu_person_lacks_side0_flag(ds, person) };
  if (record.role & Bits(PersonRole::ROLE_MASK))
    unit = opeu_find_side_selected_unit(ds, side);
  else if (record.assignment & Bits(Assignment::DIVISION))
    unit = opeu_find_group_selected_unit(
        ds,
        Near(data,
             data.groups[(record.assignment & Bits(Assignment::DIVISION_MASK)) +
                         side * GROUPS_PER_SIDE]));
  return unit;
}
