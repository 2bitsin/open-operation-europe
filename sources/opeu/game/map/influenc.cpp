// The signed 10 x 10 influence map, rebuilt from the strength of every unit and objective (positive for side 0,
// negative for side 1) weighted by distance, and the hourly step that rebuilds it before the request and arrival
// stages.
#include "map/influenc.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/arrivals.hpp"
#include "core/requests.hpp"
#include "map/power.hpp"
#include "map/query.hpp"
#include "types.hpp"

#include "port.hpp"

#include <array>

using namespace opeu::game;

namespace {
enum class HourlyService : uint16_t {
  REQUESTS = 0,
  ARRIVALS = 1,
};
// The request stage reads its stale-request threshold from this word below the entry SP, never set by its caller.
constexpr uint16_t STALE_THRESHOLD_AT{ 0x10 };
constexpr unsigned UNIT_COUNT{ 2 * UNITS_PER_SIDE };
constexpr uint8_t  BLOCK{ 4 }; // cells per influence square
} // namespace

void opeu_update_influence_map(uint8_t* ds) {
  auto&                                data{ Data(ds) };
  std::array<int16_t, UNIT_COUNT>      unit_strength{};
  std::array<int16_t, OBJECTIVE_COUNT> objective_strength{};
  for (unsigned i{ 0 }; i < UNIT_COUNT; ++i) {
    auto const strength{ opeu_unit_strength(ds, Near(data, data.units[i])) };
    unit_strength[i] = i >= UNITS_PER_SIDE ? static_cast<int16_t>(-strength) : strength;
  }
  for (unsigned i{ 0 }; i < OBJECTIVE_COUNT; ++i) {
    auto const objective{ Near(data, data.objectives[i]) };
    auto const strength{ opeu_objective_strength(ds, objective) };
    objective_strength[i] = opeu_objective_side(ds, objective) == 1 ? static_cast<int16_t>(-strength) : strength;
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

void opeu_turn_hourly_c(uint8_t* ds, uint16_t (*event)(uint8_t*, uint16_t, uint16_t, uint16_t, uint16_t, uint16_t)) {
  opeu_update_influence_map(ds);
  event(ds, std::to_underlying(HourlyService::REQUESTS), 0, 0, 0, 0);
  event(ds, std::to_underlying(HourlyService::ARRIVALS), 0, 0, 0, 0);
}

uint16_t svc_influenc(uint8_t* ds, uint16_t id, uint16_t a, uint16_t b, uint16_t c, uint16_t d) {
  (void)a;
  (void)b;
  (void)c;
  (void)d;
  switch (static_cast<HourlyService>(id)) {
  case HourlyService::REQUESTS: {
    auto const sp{ port_stack_enter(PORT_FRAME) };
    opeu_hourly_c_stage_b(ds, svc_requests, Word(ds, Local(sp, STALE_THRESHOLD_AT)));
    port_stack_leave(PORT_FRAME);
    break;
  }
  case HourlyService::ARRIVALS: opeu_hourly_c_stage_c(ds, svc_arrivals); break;
  default:                      port_unhandled("influenc", id, 0);
  }
  return 0;
}
