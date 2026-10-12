// Whether an objective can be supplied: it must be in communication itself or have a friendly unit in communication on
// or next to it, and the side's selected unit must be in communication.
#include "map/objeleg.hpp"
#include "common/data-segment.hpp"
#include "core/recindex.hpp"
#include "core/selunit.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "types.hpp"

#include "port.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t DIRECTIONS_AND_CENTER{ 9 }; // direction 8 stays on the cell

[[nodiscard]] auto NeighbourInCommunication(const uint8_t* ds, uint16_t objective) -> bool {
  auto const& data{ Data(ds) };
  auto const& record{ Record<Objective>(data, objective) };
  if (record.flags & Bits(ObjectiveFlag::COMMUNICATIONS)) return true;
  for (uint8_t direction{ 0 }; direction < DIRECTIONS_AND_CENTER; ++direction) {
    uint8_t x{ record.x };
    uint8_t y{ record.y };
    if (!opeu_grid_step(&x, &y, direction)) continue;
    auto const unit{ opeu_find_unit_at(ds, x, y) };
    if (unit && opeu_unit_side(unit) == opeu_objective_side(ds, objective)
        && (Record<Unit>(data, unit).flags & Bits(UnitFlag::COMMUNICATIONS)))
      return true;
  }
  return false;
}
} // namespace

uint16_t opeu_objective_eligible(const uint8_t* ds, uint16_t objective) {
  if (!NeighbourInCommunication(ds, objective)) return 0;
  return opeu_unit_support_condition(ds, opeu_find_side_selected_unit(ds, opeu_objective_side(ds, objective)));
}

auto port_routine_opeu_objective_eligible(std::uint8_t* ds, std::uint16_t const* args, unsigned count)
    -> std::uint32_t {
  return opeu_objective_eligible(ds, Arg(args, count, 0));
}
