// Converts the record indices a scenario or saved game stores into near pointers: a stored 0xffff is null, any other
// index becomes the near pointer of that record (wrapping in 16-bit arithmetic).
#include "io/rebase.hpp"

#include "common/data-segment.hpp"

#include <iterator>

using namespace opeu::game;

namespace {

constexpr std::uint16_t STORED_NULL{ 0xffff };

// The near pointer of record `index` of the table of T at `base`.
template <typename T>
auto Rebased(std::uint16_t index, std::uint16_t base) -> std::uint16_t {
  if (index == STORED_NULL) return 0;
  return static_cast<std::uint16_t>(base + index * sizeof(T));
}

// A side's order targets an objective or a unit, by its kind.
auto RebaseSideOrder(Order& order) -> void {
  switch (order.kind) {
  case 0:
  case 2:
  case 3:
  case 6: order.target = Rebased<Objective>(order.target, OBJECTIVES_AT); break;
  case 1:
  case 4:
  case 5:
  case 7: order.target = Rebased<Unit>(order.target, UNITS_AT); break;
  }
}

// Unit orders whose target is another unit.
auto TargetsUnit(Order const& order) -> bool {
  switch (order.kind) {
  case 2:
  case 9:
  case 11:
  case 12: return true;
  default: return false;
  }
}
} // namespace

// `extended` also rebases the objective and unit link tables, which only saved games store.
void opeu_rebase_scenario(std::uint8_t* ds, std::uint16_t extended) {
  auto& data{ Data(ds) };
  for (auto& side : data.sides) {
    RebaseSideOrder(side.order);
    for (auto& objective : side.objectives) objective.target = Rebased<Objective>(objective.target, OBJECTIVES_AT);
  }
  for (auto& special : data.special) special.target = Rebased<Objective>(special.target, OBJECTIVES_AT);
  if (extended) {
    // word arrays of the packed segment: indexed, as a reference cannot bind to their elements
    for (auto i{ 0u }; i < std::size(data.objective_links); i++)
      data.objective_links[i] = Rebased<Objective>(data.objective_links[i], OBJECTIVES_AT);
    for (auto i{ 0u }; i < std::size(data.unit_links); i++)
      data.unit_links[i] = Rebased<Unit>(data.unit_links[i], UNITS_AT);
  }
  for (auto& unit : data.units)
    for (auto& order : unit.orders)
      if (TargetsUnit(order)) order.target = Rebased<Unit>(order.target, UNITS_AT);
  for (auto& unit : data.units) unit.first = Rebased<Element>(unit.first, ELEMENTS_AT);
  for (auto& element : data.elements) element.next = Rebased<Element>(element.next, ELEMENTS_AT);
}
