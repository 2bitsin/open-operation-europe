// The strength of equipment, units and objectives, and the signed influence they spread over the map.
#include "map/power.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/game.hpp"
#include "core/limits.hpp"
#include "map/distance.hpp"
#include "types.hpp"

#include <algorithm>

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

// The summed strength of a ready unit's elements; 0 for a unit not on the map.
int16_t opeu_unit_strength(Game const& game, uint16_t unit) {
  auto const& data{ Data(game) };
  auto const& record{ Record<Unit>(data, unit) };
  if (!(record.flags & Bits(UnitFlag::READY))) return 0;
  return std::ranges::fold_left(Elements(data, record), int16_t{ 0 }, [&](int16_t sum, auto const& element) { return static_cast<int16_t>(sum + Strength(data, element.type, element.quantity)); });
}

// The summed strength of an objective's stock (an empty slot has none).
int16_t opeu_objective_strength(Game const& game, uint16_t objective) {
  auto const& data{ Data(game) };
  auto const& record{ Record<Objective>(data, objective) };
  if (!record.active) return 0;
  return std::ranges::fold_left(record.items, int16_t{ 0 }, [&](int16_t sum, auto const& item) { return static_cast<int16_t>(sum + Strength(data, item.type, item.quantity)); });
}

auto opeu_add_influence(std::int16_t value, std::int16_t amount) -> std::int16_t {
  return std::clamp(static_cast<std::int16_t>(value + amount), static_cast<std::int16_t>(-INFLUENCE_LIMIT), INFLUENCE_LIMIT);
}

// Full at the source's square, a fifth less per square of distance, none from five squares on. The product wraps to a
// signed 16-bit word before the division.
auto opeu_weight_influence(GridCell square, GridCell source, std::int16_t strength) -> std::int16_t {
  auto const remaining{ static_cast<std::int16_t>(INFLUENCE_REACH - static_cast<std::int16_t>(opeu_min16(GridDistance(square, source), INFLUENCE_REACH))) };
  return static_cast<std::int16_t>(static_cast<std::int16_t>(remaining * strength) / INFLUENCE_REACH);
}
