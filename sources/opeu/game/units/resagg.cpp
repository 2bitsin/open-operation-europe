// Stored resources or capacities summed over a group's or a side's active units, and the capacities of an objective's
// stock.
#include "units/resagg.hpp"
#include "common/data-segment.hpp"
#include "core/numeric.hpp"
#include "core/recindex.hpp"
#include "types.hpp"
#include "units/capacity.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

// Sums over the group's ready units: their stored resources, or their capacities when `stored` is zero.
void opeu_group_resources(Game& game, uint16_t group, uint32_t* result_a, uint32_t* result_b, uint32_t* result_c, uint16_t stored) {
  auto& data{ Data(game) };
  *result_a = *result_b = *result_c = 0;
  auto const first{ UNITS_PER_SIDE * opeu_group_side(group) + UNITS_PER_GROUP * opeu_group_index(group) };
  for (auto index{ first }; index < first + UNITS_PER_GROUP; ++index) {
    auto& unit{ data.units[index] };
    if (!(unit.flags & Bits(UnitFlag::READY))) continue;
    std::uint16_t a, b, c;
    if (stored) {
      a = unit.resources[0];
      b = unit.resources[1];
      c = unit.resources[2];
    } else
      opeu_unit_capacities(game, Near(data, unit), &a, &b, &c);
    *result_a += a;
    *result_b += b;
    *result_c += c;
  }
}

// Sums opeu_group_resources over the side's ready groups.
void opeu_side_resources(Game& game, uint8_t side, uint32_t* result_a, uint32_t* result_b, uint32_t* result_c, uint16_t stored) {
  auto& data{ Data(game) };
  *result_a = *result_b = *result_c = 0;
  for (auto index{ GROUPS_PER_SIDE * side }; index < GROUPS_PER_SIDE * (side + 1u); ++index) {
    auto& group{ data.groups[index] };
    if (!(group.flags & Bits(GroupFlag::READY))) continue;
    std::uint32_t a, b, c;
    opeu_group_resources(game, Near(data, group), &a, &b, &c, stored);
    *result_a += a;
    *result_b += b;
    *result_c += c;
  }
}

// The capacities of an objective's stock items, each sum saturating at 60000.
void opeu_objective_capacities(Game& game, uint16_t objective, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c) {
  auto& data{ Data(game) };
  *result_a = *result_b = *result_c = 0;
  for (auto const& item : Record<Objective>(data, objective).items) {
    if (!item.quantity) continue;
    auto const& type{ data.element_types[item.type] };
    auto const  category{ type.category };
    auto const  quantity{ item.quantity };
    opeu_add_word(static_cast<std::uint16_t>(unsigned{ data.ammunition_capacities[category] } * quantity), 60000, result_a);
    opeu_add_word(static_cast<std::uint16_t>(unsigned{ type.fuel_capacity } * quantity), 60000, result_b);
    opeu_add_word(static_cast<std::uint16_t>(unsigned{ data.food_capacities[category] } * quantity), 60000, result_c);
  }
}
