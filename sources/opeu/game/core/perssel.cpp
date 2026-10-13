// The unit a general currently has selected.
#include "core/perssel.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "core/personfl.hpp"
#include "core/selunit.hpp"
#include "data-segment.hpp"
#include "types.hpp"

using namespace opeu::game;

// A staff officer looks at his side's selected group, a division commander at his division; 0 for anyone else or
// when nothing is selected.
uint16_t opeu_person_selected_unit(Game const& game, uint16_t person) {
  auto const&   data{ Data(game) };
  auto const&   record{ Record<Person>(data, person) };
  uint16_t      unit{ 0 };
  uint8_t const side{ opeu_person_lacks_side0_flag(game, person) };
  if (record.role & Bits(PersonRole::ROLE_MASK))
    unit = opeu_find_side_selected_unit(game, side);
  else if (record.assignment & Bits(Assignment::DIVISION))
    unit = opeu_find_group_selected_unit(game, Near(data, GroupOf(data, side, record.assignment & Bits(Assignment::DIVISION_MASK))));
  return unit;
}
