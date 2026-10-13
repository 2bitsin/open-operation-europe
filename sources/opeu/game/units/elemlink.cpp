// Linking elements into units: stable insertion by category and removal.
#include "units/elemlink.hpp"
#include "common/data-segment.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "core/recindex.hpp"
#include "map/occupy.hpp"
#include "types.hpp"

#include <algorithm>

using namespace opeu::game;

// Inserts the element after the unit's elements of the same or a lower category, marks it ready, places the unit on
// the map and readies its division. The link walk treats the unit as a chain head: its first word is the next link.
void opeu_insert_element(Game& game, uint16_t unit, uint16_t element) {
  auto&      data{ Data(game) };
  auto const category{ ElementTypeOf(data, element).category };
  auto       at{ unit };
  for (auto next{ Record<Element>(data, at).next }; next && ElementTypeOf(data, next).category <= category; next = Record<Element>(data, at).next) at = next;
  auto& inserted{ Record<Element>(data, element) };
  inserted.next                   = Record<Element>(data, at).next;
  Record<Element>(data, at).next  = element;
  inserted.flags                 |= Bits(ElementFlag::READY);
  opeu_place_unit(game, unit);
  Record<Group>(data, opeu_unit_group(unit)).flags |= Bits(GroupFlag::READY);
}

// Unlinks the element from the unit. Returns 0 if the unit keeps elements (or the element is not in it), 1 if the
// emptied unit leaves the map but its division keeps a ready unit, 3 if the division has no ready unit left either.
uint8_t opeu_remove_element(Game& game, uint16_t unit, uint16_t element) {
  auto&      data{ Data(game) };
  auto const side{ opeu_unit_side(unit) };
  auto const group{ opeu_unit_group_index(unit) };
  for (auto at{ unit }; Record<Element>(data, at).next; at = Record<Element>(data, at).next) {
    if (Record<Element>(data, at).next != element) continue;
    auto& removed{ Record<Element>(data, element) };
    Record<Element>(data, at).next  = removed.next;
    removed.flags                  &= static_cast<std::uint8_t>(~Bits(ElementFlag::READY));
    if (Record<Unit>(data, unit).first) return 0;
    opeu_remove_unit(game, unit);
    if (std::ranges::any_of(GroupUnits(data, side, group), [](Unit const& member) { return (member.flags & Bits(UnitFlag::READY)) != 0; })) return 1;
    GroupOf(data, side, group).flags &= static_cast<std::uint8_t>(~Bits(GroupFlag::READY));
    return 3;
  }
  return 0;
}
