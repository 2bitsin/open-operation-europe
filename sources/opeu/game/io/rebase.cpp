// Converts the record indices a scenario or saved game stores into near pointers: a stored 0xffff is null, any other
// index becomes the near pointer of that record (wrapping in 16-bit arithmetic).
#include "io/rebase.hpp"

#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/saved-game.hpp"

#include <iterator>

using namespace opeu::game;

// `extended` also rebases the objective and unit link tables, which only saved games store.
void opeu_rebase_scenario(Game& game, std::uint16_t extended) {
  auto& data{ Data(game) };
  for (auto& side : data.sides) {
    if (auto const target{ SideOrderTarget(side.order) }; target != OrderTarget::NONE) side.order.target = TargetRecord(side.order.target, target);
    for (auto& objective : side.objectives) objective.target = StoredRecord<Objective>(objective.target, OBJECTIVES_AT);
  }
  for (auto& special : data.special) special.target = StoredRecord<Objective>(special.target, OBJECTIVES_AT);
  if (extended) {
    // word arrays of the packed segment: indexed, as a reference cannot bind to their elements
    for (auto i{ 0u }; i < std::size(data.objective_links); i++) data.objective_links[i] = StoredRecord<Objective>(data.objective_links[i], OBJECTIVES_AT);
    for (auto i{ 0u }; i < std::size(data.unit_links); i++) data.unit_links[i] = StoredRecord<Unit>(data.unit_links[i], UNITS_AT);
  }
  for (auto& unit : data.units)
    for (auto& order : unit.orders)
      if (UnitOrderTarget(order) == OrderTarget::UNIT) order.target = StoredRecord<Unit>(order.target, UNITS_AT);
  for (auto& unit : data.units) unit.first = StoredRecord<Element>(unit.first, ELEMENTS_AT);
  for (auto& element : data.elements) element.next = StoredRecord<Element>(element.next, ELEMENTS_AT);
}
