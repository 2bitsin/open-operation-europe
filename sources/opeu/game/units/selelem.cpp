// The first selected element in a unit's chain. Selection does not require the element to be ready.
#include "units/selelem.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "types.hpp"

using namespace opeu::game;

uint16_t opeu_unit_selected_element(Game const& game, uint16_t unit) {
  auto const& data{ Data(game) };
  auto const  chain{ Elements(data, Record<Unit>(data, unit)) };
  for (auto at{ chain.begin() }; at != chain.end(); ++at)
    if ((*at).flags & Bits(ElementFlag::SELECTED)) return at.Near();
  return 0;
}
