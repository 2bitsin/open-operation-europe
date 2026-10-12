// A resource amount as a percentage of a capacity, capped at 999 (also when the capacity is 0).
#include "units/ratio.hpp"
#include "types.hpp"

#include <cstdint>

// The product wraps at 32 bits and the quotient truncates; INT32_MIN / -1 wraps to INT32_MIN.
uint16_t opeu_resource_percent_wide(int32_t current, int32_t capacity) {
  if (!capacity) return 999;
  auto const scaled{ static_cast<std::int32_t>(static_cast<std::uint32_t>(current) * 100u) };
  auto const percent{ static_cast<std::int32_t>(std::int64_t{ scaled } / capacity) };
  return percent > 999 ? 999 : static_cast<std::uint16_t>(percent);
}

uint16_t opeu_resource_percent(uint16_t current, uint16_t capacity) {
  return opeu_resource_percent_wide(current, capacity);
}
