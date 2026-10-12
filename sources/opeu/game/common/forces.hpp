// A side's forces in the data segment: its four groups (divisions) and the eight units (regiments) of each.
#pragma once

#include "common/data-segment.hpp"

#include <cstdint>
#include <span>

namespace opeu::game::detail::forces {

// Division `group` (0..3) of `side`.
[[nodiscard]] inline auto GroupOf(DataSegment& data, unsigned side, unsigned group) noexcept -> Group& {
  return data.groups[side * GROUPS_PER_SIDE + group];
}
[[nodiscard]] inline auto GroupOf(DataSegment const& data, unsigned side, unsigned group) noexcept -> Group const& {
  return data.groups[side * GROUPS_PER_SIDE + group];
}

// The eight unit slots of division `group` of `side`, in order.
[[nodiscard]] inline auto GroupUnits(DataSegment& data, unsigned side, unsigned group) noexcept -> std::span<Unit, UNITS_PER_GROUP> {
  return std::span<Unit, UNITS_PER_GROUP>{ data.units.data() + (side * GROUPS_PER_SIDE + group) * UNITS_PER_GROUP, UNITS_PER_GROUP };
}
[[nodiscard]] inline auto GroupUnits(DataSegment const& data, unsigned side, unsigned group) noexcept -> std::span<Unit const, UNITS_PER_GROUP> {
  return std::span<Unit const, UNITS_PER_GROUP>{ data.units.data() + (side * GROUPS_PER_SIDE + group) * UNITS_PER_GROUP, UNITS_PER_GROUP };
}

// The 32 unit slots of `side`, division by division.
[[nodiscard]] inline auto SideUnits(DataSegment& data, unsigned side) noexcept -> std::span<Unit, UNITS_PER_SIDE> {
  return std::span<Unit, UNITS_PER_SIDE>{ data.units.data() + side * UNITS_PER_SIDE, UNITS_PER_SIDE };
}

// The 128 element slots of `side`.
[[nodiscard]] inline auto SideElements(DataSegment& data, unsigned side) noexcept -> std::span<Element, ELEMENTS_PER_SIDE> {
  return std::span<Element, ELEMENTS_PER_SIDE>{ data.elements.data() + side * ELEMENTS_PER_SIDE, ELEMENTS_PER_SIDE };
}

// A division that is fielded (READY) and not held back in reserve.
[[nodiscard]] inline auto IsActiveGroup(Group const& group) noexcept -> bool {
  return (group.flags & Bits(GroupFlag::READY)) && !(group.flags & Bits(GroupFlag::RESERVE));
}
} // namespace opeu::game::detail::forces

namespace opeu::game {
using detail::forces::GroupOf;
using detail::forces::GroupUnits;
using detail::forces::IsActiveGroup;
using detail::forces::SideElements;
using detail::forces::SideUnits;
} // namespace opeu::game
