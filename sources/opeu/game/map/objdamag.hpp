// The damage report of a bombed objective (map/objdamag.cpp).
#pragma once

#include "common/events.hpp"
#include "types.hpp"

namespace opeu::game::detail::objdamag {

// A bombing's damage: `chance` percent of the equipment kinds take a hit of base + rand(spread) (the others
// rand(spread / 2)); heavy bombing multiplies infantry's losses by 6 to 15; supply and endurance size the supply and
// strength losses. A base of 0 spares the equipment, a supply or endurance of 0 the stocks or the strength.
struct Bombing {
  std::uint16_t chance;
  std::uint16_t base;
  std::uint16_t spread;
  bool          heavy;
  std::uint16_t supply;
  std::uint16_t endurance;
};
} // namespace opeu::game::detail::objdamag

namespace opeu::game {
using detail::objdamag::Bombing;
} // namespace opeu::game

void opeu_objective_loss_line(Game& game, uint16_t objective, uint8_t kind, uint16_t amount);
void opeu_objective_damage(Game& game, uint16_t objective, opeu::game::Bombing const& bombing);
