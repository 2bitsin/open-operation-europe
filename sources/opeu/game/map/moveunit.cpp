// Moves the current unit one or more cells toward its order destination: it gains movement points from its rating,
// plans a route to the destination (or a cell within ten steps of it) through the path-cost grid, and steps along it
// while it can pay the cells' costs, has fuel and meets no enemy, setting off minefields and spotting on the way.
#include "map/moveunit.hpp"
#include "common/data-segment.hpp"
#include "common/game.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/limits.hpp"
#include "core/numeric.hpp"
#include "core/recindex.hpp"
#include "map/cellflag.hpp"
#include "map/distance.hpp"
#include "map/maperase.hpp"
#include "map/mappos.hpp"
#include "map/mapunit.hpp"
#include "map/movecost.hpp"
#include "map/movemine.hpp"
#include "map/movespot.hpp"
#include "map/movesup.hpp"
#include "map/movetgt.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/toward.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {
constexpr uint8_t  NONE{ 255 };
constexpr uint8_t  POINTS_LIMIT{ 99 };
constexpr uint8_t  NEAR_DESTINATION{ 3 }; // cells: replan the route from closer than this
constexpr uint8_t  ROUTE_STEPS{ 10 };     // the farthest a planned waypoint lies
constexpr uint16_t STUCK{ 0xffff };       // the step toward found no cell
// The DS reserve of the cell pairs the services move through, and their offsets in it: a probe, the unit's next cell
// and its destination.
constexpr uint16_t CELLS_RESERVE{ 0x1a };
constexpr uint16_t PROBE_X_AT{ 0x16 };
constexpr uint16_t PROBE_Y_AT{ 0x14 };
constexpr uint16_t NEXT_X_AT{ 6 };
constexpr uint16_t NEXT_Y_AT{ 4 };
constexpr uint16_t GOAL_X_AT{ 2 };
constexpr uint16_t GOAL_Y_AT{ 0 };

// A cell the caller holds in x and y that a service moves through the DS bytes at x_at and y_at: the cell is copied
// there before the call and read back after it.
struct ReservedCell {
  uint8_t& x;
  uint8_t& y;
  uint16_t x_at;
  uint16_t y_at;
};

// The cell a step heads for.
struct Goal {
  uint8_t x;
  uint8_t y;
};

auto Stage(Game& game, ReservedCell const& cell) -> void {
  Byte(game, cell.x_at) = cell.x;
  Byte(game, cell.y_at) = cell.y;
}

auto Fetch(Game& game, ReservedCell const& cell) -> void {
  cell.x = Byte(game, cell.x_at);
  cell.y = Byte(game, cell.y_at);
}

// Moves the cell to its neighbour in the direction; nonzero when the neighbour is on the map.
auto GridStep(Game& game, ReservedCell const& cell, uint8_t direction) -> uint16_t {
  Stage(game, cell);
  auto const stepped{ opeu_grid_step(game.State().Bytes(static_cast<std::uint16_t>(cell.x_at), 1).data(), game.State().Bytes(static_cast<std::uint16_t>(cell.y_at), 1).data(),
                                     static_cast<uint8_t>(direction)) };
  Fetch(game, cell);
  return stepped;
}

// Moves the cell one step along the path toward the goal; returns the step's cost, or STUCK.
auto StepToward(Game& game, ReservedCell const& cell, Goal goal) -> uint16_t {
  Stage(game, cell);
  auto const cost{ opeu_grid_step_toward(game, port_far(PATH_SEGMENT, 0), static_cast<std::uint16_t>(cell.x_at), static_cast<std::uint16_t>(cell.y_at), static_cast<uint8_t>(goal.x),
                                         static_cast<uint8_t>(goal.y)) };
  Fetch(game, cell);
  return cost;
}
} // namespace

// repeat: keep stepping while the steps cost nothing.
void opeu_move_unit(Game& game, uint16_t repeat) {
  auto&               data{ Data(game) };
  auto const          current{ data.current_unit_ptr };
  auto&               unit{ Record<Unit>(data, current) };
  ServiceLocals const locals{ CELLS_RESERVE };
  auto const          probe_x_at{ locals.At(PROBE_X_AT) };
  auto const          probe_y_at{ locals.At(PROBE_Y_AT) };
  auto const          next_x_at{ locals.At(NEXT_X_AT) };
  auto const          next_y_at{ locals.At(NEXT_Y_AT) };
  auto const          step{ [&](uint8_t& x, uint8_t& y, uint8_t direction) { return GridStep(game, { x, y, probe_x_at, probe_y_at }, direction); } };
  auto const cost_at{ [&](uint8_t x, uint8_t y) { return static_cast<uint8_t>(opeu_move_cost(game, static_cast<uint8_t>(x), static_cast<uint8_t>(y), static_cast<std::uint16_t>(current))); } };
  uint8_t    next_x{ unit.x };
  uint8_t    next_y{ unit.y };
  auto const rating{ static_cast<uint8_t>(opeu_move_rating(game)) };
  opeu_add_byte(rating, POINTS_LIMIT, &unit.moved);

  // Nothing to do unless some neighbouring cell is affordable.
  bool affordable{ false };
  for (uint8_t direction{ 0 }; direction < GRID_DIRECTIONS && !affordable; direction++) {
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
            && static_cast<uint8_t>(opeu_grid_distance(static_cast<uint8_t>(next_x), static_cast<uint8_t>(next_y), static_cast<uint8_t>(unit.target_x), static_cast<uint8_t>(unit.target_y)))
                   < NEAR_DESTINATION)) {
      // The destination lands in its DS bytes; where the unit has none they keep what they held.
      opeu_unit_order_destination(game, static_cast<std::uint16_t>(current), game.State().Bytes(locals.At(GOAL_X_AT), 1).data(), game.State().Bytes(locals.At(GOAL_Y_AT), 1).data());
      uint8_t const goal_x{ Byte(game, locals.At(GOAL_X_AT)) };
      uint8_t const goal_y{ Byte(game, locals.At(GOAL_Y_AT)) };
      if (!opeu_move_reachable(game, reinterpret_cast<uint16_t*>(port_far(PATH_SEGMENT, 0)), static_cast<uint8_t>(goal_x), static_cast<uint8_t>(goal_y), static_cast<uint8_t>(next_x),
                               static_cast<uint8_t>(next_y), static_cast<std::uint16_t>(current))) {
        auto& link{ data.unit_links[data.current_side * GROUPS_PER_SIDE + data.current_group] };
        if (!link || opeu_category_6(static_cast<uint8_t>(data.current_entry))) link = current;
        unit.target_x = NONE;
        return;
      }
      uint8_t waypoint_x{ next_x };
      uint8_t waypoint_y{ next_y };
      for (uint8_t steps{ 0 }; (waypoint_x != goal_x || waypoint_y != goal_y) && steps < ROUTE_STEPS; steps++) StepToward(game, { waypoint_x, waypoint_y, probe_x_at, probe_y_at }, { goal_x, goal_y });
      unit.target_x = waypoint_x;
      unit.target_y = waypoint_y;
      committed     = true;
    } else if (!committed) {
      if (!opeu_move_reachable(game, reinterpret_cast<uint16_t*>(port_far(PATH_SEGMENT, 0)), static_cast<uint8_t>(unit.target_x), static_cast<uint8_t>(unit.target_y), static_cast<uint8_t>(next_x),
                               static_cast<uint8_t>(next_y), static_cast<std::uint16_t>(current))) {
        unit.target_x = NONE;
        restart       = true;
        continue;
      }
      committed = true;
    }

    opeu_move_clear(port_far(PATH_SEGMENT, 0), static_cast<uint8_t>(next_x), static_cast<uint8_t>(next_y));
    remaining = StepToward(game, { next_x, next_y, next_x_at, next_y_at }, { unit.target_x, unit.target_y });
    if (remaining == STUCK || (!repeat && !remaining)) return;
    auto const objective{ opeu_find_objective_at(game, static_cast<uint8_t>(next_x), static_cast<uint8_t>(next_y)) };
    bool       blocked{ objective != 0 && static_cast<uint8_t>(opeu_objective_side(game, objective)) != data.current_side };
    if (!blocked) {
      auto const other{ opeu_find_unit_at(game, static_cast<uint8_t>(next_x), static_cast<uint8_t>(next_y)) };
      blocked = other != 0 && static_cast<uint8_t>(opeu_unit_side(other)) != data.current_side;
    }
    if (blocked) {
      opeu_move_clear(port_far(PATH_SEGMENT, 0), static_cast<uint8_t>(next_x), static_cast<uint8_t>(next_y));
      next_x = unit.x;
      next_y = unit.y;
      continue;
    }

    auto const cost{ cost_at(next_x, next_y) };
    unit.moved = static_cast<uint8_t>(opeu_min16(opeu_max16(cost, rating), unit.moved));
    if (unit.moved < cost || static_cast<uint8_t>(opeu_cell_side(game, static_cast<uint8_t>(next_x), static_cast<uint8_t>(next_y))) != NONE) return;
    if (!opeu_move_supplied(game)) {
      unit.moved = 0;
      return;
    }
    opeu_arrival_location(game, static_cast<uint8_t>(unit.x), static_cast<uint8_t>(unit.y), static_cast<uint8_t>(data.current_side));
    if (!shown) {
      opeu_arrival_view(game, static_cast<std::uint16_t>(current));
      shown = true;
    }
    auto const from_x{ unit.x };
    auto const from_y{ unit.y };
    opeu_unit_remove_draw(game, static_cast<std::uint16_t>(current));
    unit.x      = next_x;
    unit.y      = next_y;
    unit.moved -= cost;
    opeu_arrival_prepare(game, static_cast<std::uint16_t>(current));
    opeu_move_clamp(game);
    if (opeu_cell_marked(game, static_cast<uint8_t>(next_x), static_cast<uint8_t>(next_y), static_cast<uint8_t>(NONE))) {
      opeu_move_mines(game, static_cast<uint8_t>(from_x), static_cast<uint8_t>(from_y));
      if (!unit.first) return;
    } else {
      opeu_move_spot(game);
    }
    opeu_arrival_prepare(game, static_cast<std::uint16_t>(current));
    opeu_move_consume(game);
    if (opeu_move_enemy_adjacent(game)) return;
    if (!remaining) return;
  }
}
