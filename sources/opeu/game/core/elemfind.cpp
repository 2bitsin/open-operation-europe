// The unit an element belongs to.
#include "core/elemfind.hpp"
#include "types.hpp"

#include "common/data-segment.hpp"
#include "common/element-chain.hpp"

using namespace opeu::game;

// The near pointer of the first ready unit whose element chain holds the element, 0 when none does. A chain must
// end; a cyclic one never returns.
uint16_t opeu_find_element_unit(const uint8_t ds[65536], uint16_t element) {
  auto const& data{ Data(ds) };
  for (auto const& unit : data.units) {
    if (!(unit.flags & Bits(UnitFlag::READY))) continue;
    for (auto it{ Elements(data, unit).begin() }; it != std::default_sentinel; ++it)
      if (it.Near() == element) return Near(data, unit);
  }
  return 0;
}
