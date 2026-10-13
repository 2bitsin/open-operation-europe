// Selection recovery for divisions, units and elements: select the first ready one, or the preferred element.
#include "units/select.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "core/recindex.hpp"
#include "types.hpp"

using namespace opeu::game;

// Selects the side's first ready division; keeps its other flags and any other selection.
void opeu_select_first_group(uint8_t ds[65536], uint8_t side) {
  auto& data{ Data(ds) };
  for (std::uint8_t index{ 0 }; index < GROUPS_PER_SIDE; ++index) {
    auto& group{ GroupOf(data, side, index) };
    if (group.flags & Bits(GroupFlag::READY)) {
      group.flags |= Bits(GroupFlag::SELECTED);
      break;
    }
  }
}

// Selects the division's first ready unit.
void opeu_select_first_unit(uint8_t ds[65536], uint16_t group) {
  auto&      data{ Data(ds) };
  auto const first{ UNITS_PER_SIDE * opeu_group_side(group) + UNITS_PER_GROUP * opeu_group_index(group) };
  for (std::uint8_t index{ 0 }; index < UNITS_PER_GROUP; ++index) {
    auto& unit{ data.units[first + index] };
    if (unit.flags & Bits(UnitFlag::READY)) {
      unit.flags |= Bits(UnitFlag::SELECTED);
      break;
    }
  }
}

// Selects the unit's element of the lowest category rank; ties keep the first element.
void opeu_select_preferred_element(uint8_t ds[65536], uint16_t unit) {
  auto&        data{ Data(ds) };
  Element*     preferred{ nullptr };
  std::uint8_t best{ 255 };
  for (auto& element : Elements(data, Record<Unit>(data, unit))) {
    auto const rank{ data.select_category_ranks[data.element_types[element.type].category] };
    if (rank < best) {
      best      = rank;
      preferred = &element;
    }
  }
  if (preferred) preferred->flags |= Bits(ElementFlag::SELECTED);
}
