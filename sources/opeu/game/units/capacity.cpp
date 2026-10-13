// The three resource capacities of a unit (ammunition, fuel, food): each element's quantity times its type's weights,
// summed in wrapping 16-bit words.
#include "units/capacity.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
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

} // namespace opeu::game::detail::capacity
