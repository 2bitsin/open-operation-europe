// A unit's ammunition, fuel and food as percentages of what its elements can carry.
#include "types.hpp"
#include "units/percent.hpp"
#include "common/data-segment.hpp"
#include "units/capacity.hpp"
#include "units/ratio.hpp"

using namespace opeu::game;

void opeu_unit_resource_percentages(const uint8_t ds[65536], uint16_t unit,
                                    uint16_t* result_a, uint16_t* result_b,
                                    uint16_t* result_c) {
  uint16_t capacity_c, capacity_b, capacity_a;
  opeu_unit_capacities(ds, unit, &capacity_a, &capacity_b, &capacity_c);
  auto const& record{ Record<Unit>(Data(ds), unit) };
  *result_a = opeu_resource_percent(record.resources[0], capacity_a);
  *result_b = opeu_resource_percent(record.resources[1], capacity_b);
  *result_c = opeu_resource_percent(record.resources[2], capacity_c);
}
