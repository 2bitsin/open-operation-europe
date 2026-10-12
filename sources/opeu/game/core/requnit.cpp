// Building the regiment a request brings: the side's marked unit slot gets the marked battalions.
#include "core/requnit.hpp"
#include "common/forces.hpp"
#include "common/random.hpp"
#include "core/numeric.hpp"
#include "data-segment.hpp"
#include "types.hpp"
#include "units/capacity.hpp"
#include "units/elemlink.hpp"
#include "units/orders.hpp"

using namespace opeu::game;

namespace {

constexpr uint8_t ORDER_SLOTS{ 3 }; // the unit's order slots cleared for the new regiment

// A fresh regiment's supplies: 30..79 percent of what it can carry.
auto InitialSupply(uint8_t* ds, uint16_t capacity) -> uint16_t {
  return opeu_percent(capacity, RandomBelow(ds, 50) + 30);
}

} // namespace

// Places the regiment at the request's cell (request_x, request_y) in the first marked unit slot of the side, links
// every marked battalion of the side into it, selects its first battalion and gives it part of its supplies. Returns
// the unit's near pointer. The game always marks a slot; without one the record after the side's last unit is used.
uint16_t opeu_build_request_unit(uint8_t ds[65536], uint8_t side) {
  auto& data{ Data(ds) };
  auto  unit_at{ Near(data, SideUnits(data, side).back()) + sizeof(Unit) };
  for (uint8_t group = 0; group < GROUPS_PER_SIDE; ++group) {
    for (auto& unit : GroupUnits(data, side, group)) {
      if (!(unit.flags & Bits(UnitFlag::REQUESTED))) continue;
      unit.x     = data.request_x;
      unit.y     = data.request_y;
      unit.first = 0;
      unit.flags = Bits(UnitFlag::READY) | Bits(UnitFlag::COMMUNICATIONS);
      if (!(GroupOf(data, side, group).flags & Bits(GroupFlag::READY))) unit.flags |= Bits(UnitFlag::SELECTED);
      unit_at = Near(data, unit);
      opeu_clear_unit_orders(ds, unit_at, ORDER_SLOTS);
      goto found;
    }
  }
found:
  auto const near{ static_cast<uint16_t>(unit_at) };
  for (auto& element : SideElements(data, side))
    if (element.flags & Bits(ElementFlag::REQUESTED)) {
      element.flags = Bits(ElementFlag::READY);
      opeu_insert_element(ds, near, Near(data, element));
    }
  auto& unit{ Record<Unit>(data, near) };
  Record<Element>(data, unit.first).flags |= Bits(ElementFlag::SELECTED);
  uint16_t ammunition, fuel, food;
  opeu_unit_capacities(ds, near, &ammunition, &fuel, &food);
  unit.resources[0] = InitialSupply(ds, ammunition);
  unit.resources[1] = InitialSupply(ds, fuel);
  unit.resources[2] = InitialSupply(ds, food);
  return near;
}
