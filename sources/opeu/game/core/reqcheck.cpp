// Requests against the records: the objective slot a delivery fills, the marks of a requested regiment.
#include "types.hpp"
#include "core/reqcheck.hpp"
#include "core/recindex.hpp"
#include "data-segment.hpp"
#include "common/forces.hpp"

using namespace opeu::game;

// The near pointer of the objective's item slot that takes a delivery of `type`: the first empty slot, or the first
// that holds the type below the capacity of its category; 0 when every slot is taken.
uint16_t opeu_request_slot(uint8_t ds[65536], uint16_t objective,
                           uint8_t type) {
  auto&      data{ Data(ds) };
  auto const capacity{
    data.request_capacities[data.element_types[type].category]
  };
  for (auto& item : Record<Objective>(data, objective).items)
    if (!item.quantity || (item.type == type && item.quantity < capacity))
      return Near(data, item);
  return 0;
}

// Clears the request marks of the side's units and elements.
void opeu_request_complete(uint8_t ds[65536], uint8_t side) {
  auto& data{ Data(ds) };
  for (auto& unit : SideUnits(data, side))
    unit.flags &= ~Bits(UnitFlag::REQUESTED);
  for (auto& element : SideElements(data, side))
    element.flags &= ~Bits(ElementFlag::REQUESTED);
}

// Whether the division of the side's marked unit slot is fielded. With no marked slot the result is the near pointer
// of the side's first unit, which callers take as true.
uint16_t opeu_request_group_ready(uint8_t ds[65536], uint8_t side) {
  auto& data{ Data(ds) };
  for (auto& unit : SideUnits(data, side))
    if (unit.flags & Bits(UnitFlag::REQUESTED))
      return (Record<Group>(data, opeu_unit_group(Near(data, unit))).flags &
              Bits(GroupFlag::READY))
                 ? 1
                 : 0;
  return Near(data, SideUnits(data, side).front());
}
