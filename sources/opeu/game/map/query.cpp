// What stands on a strategic map cell: the ready unit there, the objective there, and the side holding an objective.
#include "map/query.hpp"
#include "common/data-segment.hpp"
#include "map/grid.hpp"
#include "map/occupy.hpp"
#include "types.hpp"

#include "port.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t NO_SIDE{ 255 };
}

// The near pointer to the ready unit at (x, y), 0 when the cell's occupant bits say it is empty or none is found.
uint16_t opeu_find_unit_at(const uint8_t* ds, uint8_t x, uint8_t y) {
  if (opeu_cell_side(ds, x, y) == NO_SIDE) return 0;
  auto const& data{ Data(ds) };
  for (auto const& unit : data.units)
    if ((unit.flags & Bits(UnitFlag::READY)) && unit.x == x && unit.y == y) return Near(data, unit);
  return 0;
}

// The near pointer to the objective at (x, y); only a city cell has one.
uint16_t opeu_find_objective_at(const uint8_t* ds, uint8_t x, uint8_t y) {
  if (opeu_map_code(ds, x, y) != Bits(Terrain::CITY)) return 0;
  auto const& data{ Data(ds) };
  for (auto const& objective : data.objectives)
    if (objective.active && objective.x == x && objective.y == y) return Near(data, objective);
  return 0;
}

uint8_t opeu_objective_side(const uint8_t* ds, uint16_t objective) {
  return (Record<Objective>(Data(ds), objective).flags & Bits(ObjectiveFlag::SIDE_1)) != 0;
}

auto port_routine_opeu_find_objective_at(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_find_objective_at(ds, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)));
}

auto port_routine_opeu_find_unit_at(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_find_unit_at(ds, static_cast<uint8_t>(Arg(args, count, 0)), static_cast<uint8_t>(Arg(args, count, 1)));
}

auto port_routine_opeu_objective_side(std::uint8_t* ds, std::uint16_t const* args, unsigned count) -> std::uint32_t {
  return opeu_objective_side(ds, Arg(args, count, 0));
}
