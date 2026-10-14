// The signed 10 x 10 influence map, rebuilt from the strength of every unit and objective (positive for side 0,
// negative for side 1) weighted by distance, and the hourly step that rebuilds it before the request and arrival
// stages.
#include "map/influenc.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "core/arrivals.hpp"
#include "core/requests.hpp"
#include "map/power.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

#include <algorithm>
#include <array>
#include <ranges>
#include <span>

using namespace opeu::game;

namespace {
constexpr unsigned UNIT_COUNT{ 2 * UNITS_PER_SIDE };
constexpr uint8_t  BLOCK{ 4 }; // cells per influence square
} // namespace

namespace {
// The signed strength of every unit: positive for side 0, negative for side 1.
[[nodiscard]] auto UnitStrengths(Game const& game) -> std::array<int16_t, UNIT_COUNT> {
  auto const&                     data{ Data(game) };
  std::array<int16_t, UNIT_COUNT> strengths{};
  for (auto&& [index, record, strength] : std::views::zip(std::views::iota(0u), data.units, strengths)) {
    auto const own{ opeu_unit_strength(game, Near(data, record)) };
    strength = index >= UNITS_PER_SIDE ? static_cast<int16_t>(-own) : own;
  }
  return strengths;
}

// The signed strength of every objective: negative when side 1 holds it.
[[nodiscard]] auto ObjectiveStrengths(Game const& game) -> std::array<int16_t, OBJECTIVE_COUNT> {
  auto const&                          data{ Data(game) };
  std::array<int16_t, OBJECTIVE_COUNT> strengths{};
  std::ranges::transform(data.objectives, strengths.begin(), [&](Objective const& record) {
    auto const objective{ Near(data, record) };
    auto const strength{ opeu_objective_strength(game, objective) };
    return opeu_objective_side(game, objective) == 1 ? static_cast<int16_t>(-strength) : strength;
  });
  return strengths;
}

// The influence on a square of the strengths placed at the records' cells, added to what it holds.
template <typename _Records>
[[nodiscard]] auto Spread(GridCell square, _Records const& records, std::span<int16_t const> strengths, int16_t influence) -> int16_t {
  for (auto const& [record, strength] : std::views::zip(records, strengths))
    if (strength) influence = opeu_add_influence(influence, opeu_weight_influence(square, { static_cast<uint8_t>(record.x / BLOCK), static_cast<uint8_t>(record.y / BLOCK) }, strength));
  return influence;
}
} // namespace

void opeu_update_influence_map(Game& game) {
  auto&      data{ Data(game) };
  auto const unit_strength{ UnitStrengths(game) };
  auto const objective_strength{ ObjectiveStrengths(game) };
  for (auto const y : std::views::iota(uint8_t{ 0 }, INFLUENCE_WIDTH))
    for (auto const x : std::views::iota(uint8_t{ 0 }, INFLUENCE_WIDTH)) {
      GridCell const square{ x, y };
      auto const     from_units{ Spread(square, data.units, unit_strength, 0) };
      data.influence_map[y * INFLUENCE_WIDTH + x] = Spread(square, data.objectives, objective_strength, from_units);
    }
}

void opeu_turn_hourly_c(Game& game) {
  opeu_update_influence_map(game);
  opeu_hourly_c_stage_b(game);
  opeu_hourly_c_stage_c(game);
}
