// The first selected element in a unit's chain. Selection does not require the element to be ready.
#include "types.hpp"
#include "units/selelem.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"

using namespace opeu::game;

uint16_t opeu_unit_selected_element(const uint8_t ds[65536], uint16_t unit) {
  auto const& data{ Data(ds) };
  auto const  chain{ Elements(data, Record<Unit>(data, unit)) };
  for (auto at{ chain.begin() }; at != chain.end(); ++at)
    if ((*at).flags & Bits(ElementFlag::SELECTED))
      return at.Near();
  return 0;
}
