// The selected unit of a division or of a side.
#include "core/selunit.hpp"
#include "core/recindex.hpp"
#include "data-segment.hpp"
#include "types.hpp"

using namespace opeu::game;

// Near pointer of the first ready, selected unit of the division at near pointer group; 0 when there is none.
uint16_t opeu_find_group_selected_unit(const uint8_t ds[65536], uint16_t group) {
  auto const& data{ Data(ds) };
  auto const  first{ UNITS_PER_SIDE * opeu_group_side(group) + UNITS_PER_GROUP * opeu_group_index(group) };
  for (auto i{ first }; i < first + UNITS_PER_GROUP; ++i) {
    auto const& unit{ data.units[i] };
    if ((unit.flags & Bits(UnitFlag::READY)) && (unit.flags & Bits(UnitFlag::SELECTED))) return Near(data, unit);
  }
  return 0;
}

// The selected unit of the side's first ready, selected division. A selected division without a selected unit ends
// the search with 0.
uint16_t opeu_find_side_selected_unit(const uint8_t ds[65536], uint8_t side) {
  auto const& data{ Data(ds) };
  for (auto i{ side * GROUPS_PER_SIDE }; i < (side + 1u) * GROUPS_PER_SIDE; ++i) {
    auto const& group{ data.groups[i] };
    if ((group.flags & Bits(GroupFlag::READY)) && (group.flags & Bits(GroupFlag::SELECTED))) return opeu_find_group_selected_unit(ds, Near(data, group));
  }
  return 0;
}
