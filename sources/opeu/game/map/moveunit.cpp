// Moves the current unit one or more cells toward its order destination: it gains movement points from its rating,
// plans a route to the destination (or a cell within ten steps of it) through the path-cost grid, and steps along it
// while it can pay the cells' costs, has fuel and meets no enemy, setting off minefields and spotting on the way.
#include "map/moveunit.hpp"
#include "common/data-segment.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "map/movetgt.hpp"
#include "types.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t  NONE{ 255 };
constexpr uint8_t  IMPASSABLE_COST{ 255 };
constexpr uint8_t  POINTS_LIMIT{ 99 };
constexpr uint8_t  DIRECTIONS{ 8 };
constexpr uint8_t  NEAR_DESTINATION{ 3 }; // cells: replan the route from closer than this
constexpr uint8_t  ROUTE_STEPS{ 10 }; // the farthest a planned waypoint lies
constexpr uint16_t STUCK{ 0xffff };   // the step toward found no cell
// The cell pairs below the frame that the services move through: a probe, the unit's next cell and its destination.
constexpr uint16_t PROBE_X_AT{ 4 };
constexpr uint16_t PROBE_Y_AT{ 6 };
constexpr uint16_t NEXT_X_AT{ 0x14 };
constexpr uint16_t NEXT_Y_AT{ 0x16 };
constexpr uint16_t GOAL_X_AT{ 0x18 };
constexpr uint16_t GOAL_Y_AT{ 0x1a };

// Raises a service that updates a cell through the DS bytes at x_at and y_at: the cell is copied there before the
// call and read back after it.
template <typename... Words>
auto ThroughCell(uint8_t* ds, HostEvent event, MoveService service, uint8_t& x, uint16_t x_at, uint8_t& y,
                 uint16_t y_at, Words... words) -> uint16_t {
  Byte(ds, x_at) = x;
  Byte(ds, y_at) = y;
  auto const result{ Call(ds, event, service, words...) };
  x = Byte(ds, x_at);
  y = Byte(ds, y_at);
  return result;
}
} // namespace

// frame: this routine's frame. repeat: keep stepping while the steps cost nothing.
void opeu_move_unit(uint8_t* ds, HostEvent event, uint16_t frame, uint16_t repeat) {
  auto&      data{ Data(ds) };
  auto const current{ data.current_unit_ptr };
  auto&      unit{ Record<Unit>(data, current) };
  auto const probe_x_at{ Local(frame, PROBE_X_AT) };
  auto const probe_y_at{ Local(frame, PROBE_Y_AT) };
  auto const next_x_at{ Local(frame, NEXT_X_AT) };
  auto const next_y_at{ Local(frame, NEXT_Y_AT) };
  auto const step{ [&](uint8_t& x, uint8_t& y, uint8_t direction) {
    return ThroughCell(ds, event, MoveService::GRID_STEP, x, probe_x_at, y, probe_y_at, probe_x_at, probe_y_at,
                       direction);
  } };
  auto const cost_at{ [&](uint8_t x, uint8_t y) {
    return static_cast<uint8_t>(Call(ds, event, MoveService::COST, x, y, current));
  } };
  uint8_t    next_x{ unit.x };
  uint8_t    next_y{ unit.y };
  auto const rating{ static_cast<uint8_t>(Call(ds, event, MoveService::RATING)) };
  opeu_add_byte(rating, POINTS_LIMIT, &unit.moved);

  // Nothing to do unless some neighbouring cell is affordable.
  bool affordable{ false };
  for (uint8_t direction{ 0 }; direction < DIRECTIONS && !affordable; direction++) {
    uint8_t x{ next_x };
    uint8_t y{ next_y };
    if (step(x, y, direction)) {
      auto const cost{ cost_at(x, y) };
      affordable = cost != IMPASSABLE_COST && cost <= unit.moved;
    }
  }
  if (!affordable) return;

  bool     shown{ false };     // the arrival view has been shown
  bool     committed{ false }; // the route is planned or confirmed
  bool     restart{ true };
  uint16_t remaining{ 0 };
  for (;;) {
    if (restart) {
      shown     = false;
      committed = false;
      restart   = false;
    }
    if (unit.target_x == NONE
        || (!committed
            && static_cast<uint8_t>(
                   Call(ds, event, MoveService::DISTANCE, next_x, next_y, unit.target_x, unit.target_y))
                   < NEAR_DESTINATION)) {
      // The destination lands in its DS bytes; where the unit has none they keep what they held.
      Call(ds, event, MoveService::DESTINATION, current, Local(frame, GOAL_X_AT), Local(frame, GOAL_Y_AT));
      uint8_t const goal_x{ Byte(ds, Local(frame, GOAL_X_AT)) };
      uint8_t const goal_y{ Byte(ds, Local(frame, GOAL_Y_AT)) };
      if (!Call(ds, event, MoveService::REACHABLE, goal_x, goal_y, next_x, next_y, current)) {
        auto& link{ data.unit_links[data.current_side * GROUPS_PER_SIDE + data.current_group] };
        if (!link || Call(ds, event, MoveService::IS_ENGINEER, data.current_entry)) link = current;
        unit.target_x = NONE;
        return;
      }
      uint8_t waypoint_x{ next_x };
      uint8_t waypoint_y{ next_y };
      for (uint8_t steps{ 0 }; (waypoint_x != goal_x || waypoint_y != goal_y) && steps < ROUTE_STEPS; steps++)
        ThroughCell(ds, event, MoveService::STEP_TOWARD, waypoint_x, probe_x_at, waypoint_y, probe_y_at, probe_x_at,
                    probe_y_at, goal_x, goal_y);
      unit.target_x = waypoint_x;
      unit.target_y = waypoint_y;
      committed     = true;
    } else if (!committed) {
      if (!Call(ds, event, MoveService::REACHABLE, unit.target_x, unit.target_y, next_x, next_y, current)) {
        unit.target_x = NONE;
        restart       = true;
        continue;
      }
      committed = true;
    }

    Call(ds, event, MoveService::CLEAR_PATH, next_x, next_y);
    remaining = ThroughCell(ds, event, MoveService::STEP_TOWARD, next_x, next_x_at, next_y, next_y_at, next_x_at,
                            next_y_at, unit.target_x, unit.target_y);
    if (remaining == STUCK || (!repeat && !remaining)) return;
    auto const objective{ Call(ds, event, MoveService::FIND_OBJECTIVE, next_x, next_y) };
    bool       blocked{ objective != 0
                        && static_cast<uint8_t>(Call(ds, event, MoveService::OBJECTIVE_SIDE, objective))
                               != data.current_side };
    if (!blocked) {
      auto const other{ Call(ds, event, MoveService::FIND_UNIT, next_x, next_y) };
      blocked = other != 0 && static_cast<uint8_t>(Call(ds, event, MoveService::UNIT_SIDE, other)) != data.current_side;
    }
    if (blocked) {
      Call(ds, event, MoveService::CLEAR_PATH, next_x, next_y);
      next_x = unit.x;
      next_y = unit.y;
      continue;
    }

    auto const cost{ cost_at(next_x, next_y) };
    unit.moved = static_cast<uint8_t>(opeu_min16(opeu_max16(cost, rating), unit.moved));
    if (unit.moved < cost || static_cast<uint8_t>(Call(ds, event, MoveService::CELL_SIDE, next_x, next_y)) != NONE)
      return;
    if (!Call(ds, event, MoveService::SUPPLIED)) {
      unit.moved = 0;
      return;
    }
    Call(ds, event, MoveService::ARRIVAL_LOCATION, unit.x, unit.y, data.current_side);
    if (!shown) {
      Call(ds, event, MoveService::ARRIVAL_VIEW, current);
      shown = true;
    }
    auto const from_x{ unit.x };
    auto const from_y{ unit.y };
    Call(ds, event, MoveService::UNIT_ERASE, current);
    unit.x      = next_x;
    unit.y      = next_y;
    unit.moved -= cost;
    Call(ds, event, MoveService::ARRIVAL_PREPARE, current);
    Call(ds, event, MoveService::CLAMP);
    if (Call(ds, event, MoveService::CELL_MARKED, next_x, next_y, NONE)) {
      Call(ds, event, MoveService::MINES, from_x, from_y);
      if (!unit.first) return;
    } else {
      Call(ds, event, MoveService::SPOT);
    }
    Call(ds, event, MoveService::ARRIVAL_PREPARE, current);
    Call(ds, event, MoveService::CONSUME);
    if (Call(ds, event, MoveService::ENEMY_ADJACENT)) return;
    if (!remaining) return;
  }
}

uint16_t svc_moveunit(uint8_t* ds, uint16_t id, const uint16_t* args, uint16_t count) {
  return svc_map_move(ds, "moveunit", id, args, count);
}
