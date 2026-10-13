// The strength of equipment, units and objectives, and the signed influence they spread over the map.
#include "map/power.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "core/limits.hpp"
#include "map/distance.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr int16_t INFLUENCE_LIMIT{ 20000 };
constexpr int16_t INFLUENCE_REACH{ 5 }; // cells; the weight falls by a fifth per cell

// The strength of a quantity of one equipment type: its three combat ratings and its speed weigh the quantity.
[[nodiscard]] auto Strength(DataSegment const& data, uint8_t type, uint16_t quantity) -> uint16_t {
  auto const& kind{ data.element_types[type] };
  auto const  ratings{ static_cast<uint16_t>((kind.anti_tank + kind.anti_personnel + kind.defense) / 30) };
  return static_cast<uint16_t>(ratings * (kind.speed / 10) * (quantity / 10));
}
} // namespace

// item: the near pointer to an Item (or to the type and quantity of an Element, which have the same layout).
uint16_t opeu_item_strength(Game const& game, uint16_t item) {
  auto const& data{ Data(game) };
  auto const& record{ Record<Item>(data, item) };
  return Strength(data, record.type, record.quantity);
}

// The summed strength of a ready unit's elements; 0 for a unit not on the map.
int16_t opeu_unit_strength(Game const& game, uint16_t unit) {
  auto const& data{ Data(game) };
  auto const& record{ Record<Unit>(data, unit) };
  int16_t     strength{ 0 };
  if (record.flags & Bits(UnitFlag::READY))
    for (auto const& element : Elements(data, record)) strength = static_cast<int16_t>(strength + Strength(data, element.type, element.quantity));
  return strength;
}

// The summed strength of an objective's stock.
int16_t opeu_objective_strength(Game const& game, uint16_t objective) {
  auto const& data{ Data(game) };
  auto const& record{ Record<Objective>(data, objective) };
  int16_t     strength{ 0 };
  if (record.active)
    for (auto const& item : record.items)
      if (item.quantity) strength = static_cast<int16_t>(strength + Strength(data, item.type, item.quantity));
  return strength;
}

// Adds to an influence value, clamped to +-20000.
void opeu_add_influence(int16_t amount, int16_t* value) {
  *value = static_cast<int16_t>(*value + amount);
  if (*value > INFLUENCE_LIMIT) *value = INFLUENCE_LIMIT;
  if (*value < -INFLUENCE_LIMIT) *value = -INFLUENCE_LIMIT;
}

// The share of value that reaches (x2, y2) from (x, y): full at the cell, a fifth less per cell of distance, none from
// five cells on. The product wraps to a signed 16-bit word before the division.
int16_t opeu_weight_influence(uint8_t x, uint8_t y, uint8_t x2, uint8_t y2, int16_t value) {
  auto const remaining{ static_cast<int16_t>(INFLUENCE_REACH - static_cast<int16_t>(opeu_min16(opeu_grid_distance(x, y, x2, y2), INFLUENCE_REACH))) };
  return static_cast<int16_t>(static_cast<int16_t>(remaining * value) / INFLUENCE_REACH);
}
