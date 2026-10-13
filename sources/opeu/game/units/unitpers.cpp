// The general commanding a unit: a division flagged AI_MARK uses the general assigned to the division, otherwise the
// side's role-1 general.
#include "units/unitpers.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "core/people.hpp"
#include "core/recindex.hpp"
#include "types.hpp"

using namespace opeu::game;

uint16_t opeu_unit_person(Game const& game, uint16_t unit) {
  if (Record<Group>(Data(game), opeu_unit_group(unit)).flags & Bits(GroupFlag::AI_MARK)) return opeu_find_group_record(game, opeu_unit_side(unit), opeu_unit_group_index(unit));
  return opeu_find_side_record(game, opeu_unit_side(unit), 1);
}
