// Finding a side's general by staff role or by division.
#include "core/people.hpp"
#include "common/forces.hpp"
#include "data-segment.hpp"
#include "types.hpp"

#include <span>

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

namespace { } // namespace

// Near pointer of the side's first general whose staff role is kind; 0 when none is.
uint16_t opeu_find_side_record(Game const& game, uint8_t side, uint8_t kind) {
  auto const& data{ Data(game) };
  for (auto const& person : SideGenerals(data, side))
    if (person.status != PERSON_VACANT && (person.role & Bits(PersonRole::ROLE_MASK)) == kind) return Near(data, person);
  return 0;
}

// Near pointer of the side's general commanding division group; 0 when none does.
uint16_t opeu_find_group_record(Game const& game, uint8_t side, uint8_t group) {
  auto const& data{ Data(game) };
  for (auto const& person : SideGenerals(data, side)) {
    uint8_t const assignment{ person.assignment };
    if (person.status != PERSON_VACANT && (assignment & Bits(Assignment::DIVISION)) && (assignment & Bits(Assignment::DIVISION_MASK)) == group) return Near(data, person);
  }
  return 0;
}
