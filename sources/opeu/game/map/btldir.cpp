// Battle facing and distance: the facing from a cell toward another, the facing from one element toward another
// along the dominant axis, and the grid (Chebyshev) distance between two elements.
#include "map/btldir.hpp"
#include "common/battle-field.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "core/limits.hpp"
#include "map/distance.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"
#include "services.hpp"

using namespace opeu::game;

auto CellFacingToward(CellMove move) -> uint8_t {
  auto const [from, to]{ move };
  if (to.x == from.x) return Bits(to.y < from.y ? CellFacing::UP : CellFacing::DOWN);
  return Bits(to.x < from.x ? CellFacing::LEFT : CellFacing::RIGHT);
}

uint8_t opeu_battle_element_facing(Game& game, uint16_t element, uint16_t other) {
  auto&       data{ Data(game) };
  auto const& from{ PositionOf(data, element) };
  auto const& to{ PositionOf(data, other) };
  auto const  dx{ static_cast<int16_t>(from.x - to.x) };
  auto const  dy{ static_cast<int16_t>(from.y - to.y) };
  if (static_cast<int16_t>(opeu_abs16_bits(static_cast<uint16_t>(dy))) > static_cast<int16_t>(opeu_abs16_bits(static_cast<uint16_t>(dx)))) return Bits(dy > 0 ? CellFacing::UP : CellFacing::DOWN);
  return Bits(dx > 0 ? CellFacing::LEFT : CellFacing::RIGHT);
}

uint8_t opeu_battle_terrain_mode(Game& game, uint16_t element, uint16_t other) {
  auto&       data{ Data(game) };
  auto const& from{ PositionOf(data, element) };
  auto const& to{ PositionOf(data, other) };
  return GridDistance({ from.x, from.y }, { to.x, to.y });
}
