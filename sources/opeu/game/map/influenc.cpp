// The signed 10 x 10 influence map, rebuilt from the strength of every unit and objective (positive for side 0,
// negative for side 1) weighted by distance, and the hourly step that rebuilds it before the request and arrival
// stages.
#include "map/influenc.hpp"
#include "common/data-segment.hpp"
#include "common/events.hpp"
#include "common/memory.hpp"
#include "core/arrivals.hpp"
#include "core/requests.hpp"
#include "map/power.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "common/game.hpp"
#include "port.hpp"

#include <array>

using namespace opeu::game;

namespace {
// The request stage's stale-request threshold is a DS stack word its caller never sets: the bottom word of a reserve
// this size, as the stack held it.
constexpr uint16_t STALE_THRESHOLD_RESERVE{ 0x10 };
constexpr unsigned UNIT_COUNT{ 2 * UNITS_PER_SIDE };
constexpr uint8_t  BLOCK{ 4 }; // cells per influence square
} // namespace

void opeu_update_influence_map(Game& game) {
  auto&                                data{ Data(game) };
  std::array<int16_t, UNIT_COUNT>      unit_strength{};
  std::array<int16_t, OBJECTIVE_COUNT> objective_strength{};
  for (unsigned i{ 0 }; i < UNIT_COUNT; ++i) {
    auto const strength{ opeu_unit_strength(game, Near(data, data.units[i])) };
    unit_strength[i] = i >= UNITS_PER_SIDE ? static_cast<int16_t>(-strength) : strength;
  }
  for (unsigned i{ 0 }; i < OBJECTIVE_COUNT; ++i) {
    auto const objective{ Near(data, data.objectives[i]) };
    auto const strength{ opeu_objective_strength(game, objective) };
    objective_strength[i] = opeu_objective_side(game, objective) == 1 ? static_cast<int16_t>(-strength) : strength;
  }
  for (uint8_t y{ 0 }; y < INFLUENCE_WIDTH; ++y)
    for (uint8_t x{ 0 }; x < INFLUENCE_WIDTH; ++x) {
      auto& influence{ data.influence_map[y * INFLUENCE_WIDTH + x] };
      influence = 0;
      for (unsigned i{ 0 }; i < UNIT_COUNT; ++i) {
        auto const& unit{ data.units[i] };
        if (unit_strength[i]) {
          int16_t value{ influence };
          opeu_add_influence(opeu_weight_influence(x, y, unit.x / BLOCK, unit.y / BLOCK, unit_strength[i]), &value);
          influence = value;
        }
      }
      for (unsigned i{ 0 }; i < OBJECTIVE_COUNT; ++i) {
        auto const& objective{ data.objectives[i] };
        if (objective_strength[i]) {
          int16_t value{ influence };
          opeu_add_influence(opeu_weight_influence(x, y, objective.x / BLOCK, objective.y / BLOCK, objective_strength[i]), &value);
          influence = value;
        }
      }
    }
}

namespace {
// The hour's requests, with the stale threshold the game reads from an uninitialised word of the routine's frame.
void RunRequests(Game& game) {
  ServiceLocals const locals{ STALE_THRESHOLD_RESERVE };
  opeu_hourly_c_stage_b(game, Word(game, locals.At(0)));
}
} // namespace

void opeu_turn_hourly_c(Game& game) {
  opeu_update_influence_map(game);
  RunRequests(game);
  opeu_hourly_c_stage_c(game);
}
