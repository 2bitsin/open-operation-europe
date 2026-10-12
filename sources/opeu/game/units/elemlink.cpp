// Linking elements into units: stable insertion by category and removal.
#include "types.hpp"
#include "units/elemlink.hpp"
#include "common/data-segment.hpp"
#include "core/recindex.hpp"
#include "map/occupy.hpp"

using namespace opeu::game;

namespace {
// The category of an element's equipment type.
auto CategoryOf(DataSegment const& data, std::uint16_t element)
    -> std::uint8_t {
  return data.element_types[Record<Element>(data, element).type].category;
}
} // namespace

// Inserts the element after the unit's elements of the same or a lower category, marks it ready, places the unit on
// the map and readies its division. The link walk treats the unit as a chain head: its first word is the next link.
void opeu_insert_element(uint8_t ds[65536], uint16_t unit, uint16_t element) {
  auto&      data{ Data(ds) };
  auto const category{ CategoryOf(data, element) };
  auto       at{ unit };
  for (auto next{ Record<Element>(data, at).next };
       next && CategoryOf(data, next) <= category;
       next = Record<Element>(data, at).next)
    at = next;
  auto& inserted{ Record<Element>(data, element) };
  inserted.next                  = Record<Element>(data, at).next;
  Record<Element>(data, at).next = element;
  inserted.flags |= Bits(ElementFlag::READY);
  opeu_place_unit(ds, unit);
  Record<Group>(data, opeu_unit_group(unit)).flags |= Bits(GroupFlag::READY);
}

// Unlinks the element from the unit. Returns 0 if the unit keeps elements (or the element is not in it), 1 if the
// emptied unit leaves the map but its division keeps a ready unit, 3 if the division has no ready unit left either.
uint8_t opeu_remove_element(uint8_t ds[65536], uint16_t unit,
                            uint16_t element) {
  auto&      data{ Data(ds) };
  auto const side{ opeu_unit_side(unit) };
  auto const group{ opeu_unit_group_index(unit) };
  for (auto at{ unit }; Record<Element>(data, at).next;
       at = Record<Element>(data, at).next) {
    if (Record<Element>(data, at).next != element)
      continue;
    auto& removed{ Record<Element>(data, element) };
    Record<Element>(data, at).next = removed.next;
    removed.flags &= static_cast<std::uint8_t>(~Bits(ElementFlag::READY));
    if (Record<Unit>(data, unit).first)
      return 0;
    opeu_remove_unit(ds, unit);
    auto const first{ UNITS_PER_SIDE * side + UNITS_PER_GROUP * group };
    for (std::uint8_t index{ 0 }; index < UNITS_PER_GROUP; ++index)
      if (data.units[first + index].flags & Bits(UnitFlag::READY))
        return 1;
    data.groups[GROUPS_PER_SIDE * side + group].flags &=
        static_cast<std::uint8_t>(~Bits(GroupFlag::READY));
    return 3;
  }
  return 0;
}
