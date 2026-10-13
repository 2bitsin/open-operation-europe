// The three resource capacities of a unit (ammunition, fuel, food): each element's quantity times its type's weights,
// summed in wrapping 16-bit words.
#include "units/capacity.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "common/memory.hpp"
#include "types.hpp"

namespace opeu::game::detail::capacity {

auto UnitCapacities(DataSegment const& data, std::uint16_t unit) -> Resources {
  Resources capacities{ 0, 0, 0 };
  for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
    auto const& type{ data.element_types[element.type] };
    auto const  category{ type.category };
    auto const  quantity{ element.quantity };
    capacities.ammunition += static_cast<std::uint16_t>(unsigned{ data.ammunition_capacities[category] } * quantity);
    capacities.fuel       += static_cast<std::uint16_t>(unsigned{ type.fuel_capacity } * quantity);
    capacities.food       += static_cast<std::uint16_t>(unsigned{ data.food_capacities[category] } * quantity);
  }
  return capacities;
}

auto StoreUnitCapacities(Game& game, std::uint16_t unit, ResourceWords at) -> void {
  StoreResources(game, at, UnitCapacities(Data(game), unit));
}

auto StoreResources(Game& game, ResourceWords at, Resources resources) -> void {
  SetWord(game, at.ammunition, resources.ammunition);
  SetWord(game, at.fuel, resources.fuel);
  SetWord(game, at.food, resources.food);
}
} // namespace opeu::game::detail::capacity

using namespace opeu::game;

void opeu_unit_capacities(Game const& game, uint16_t unit, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c) {
  auto const capacities{ UnitCapacities(Data(game), unit) };
  *result_a = capacities.ammunition;
  *result_b = capacities.fuel;
  *result_c = capacities.food;
}
