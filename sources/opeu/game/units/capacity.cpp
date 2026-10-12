// The three resource capacities of a unit (ammunition, fuel, food): each element's quantity times its type's weights,
// summed in wrapping 16-bit words.
#include "units/capacity.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "types.hpp"

using namespace opeu::game;

void opeu_unit_capacities(const uint8_t ds[65536], uint16_t unit, uint16_t* result_a, uint16_t* result_b, uint16_t* result_c) {
  auto const& data{ Data(ds) };
  *result_a = *result_b = *result_c = 0;
  for (auto const& element : Elements(data, Record<Unit>(data, unit))) {
    auto const& type{ data.element_types[element.type] };
    auto const  category{ type.category };
    auto const  quantity{ element.quantity };
    *result_a += static_cast<std::uint16_t>(unsigned{ data.ammunition_capacities[category] } * quantity);
    *result_b += static_cast<std::uint16_t>(unsigned{ type.fuel_capacity } * quantity);
    *result_c += static_cast<std::uint16_t>(unsigned{ data.food_capacities[category] } * quantity);
  }
}
