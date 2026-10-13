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
#include "map/path-grid.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/toward.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/value.hpp"

#include <algorithm>
#include <ranges>

using namespace opeu::game;

namespace {
constexpr uint8_t  NONE{ 255 };
constexpr uint8_t  POINTS_LIMIT{ 99 };
constexpr uint8_t  NEAR_DESTINATION{ 3 }; // cells: replan the route from closer than this
constexpr uint8_t  ROUTE_STEPS{ 10 };     // the farthest a planned waypoint lies
constexpr uint16_t STUCK{ 0xffff };       // the step toward found no cell
// The DS reserve whose bytes receive the unit's order destination: where the unit has none they keep what they held.
// It is held for the whole move, so the services the move calls reserve theirs below it.
constexpr uint16_t GOAL_RESERVE{ 0x1a };
constexpr uint16_t GOAL_X_AT{ 2 };
constexpr uint16_t GOAL_Y_AT{ 0 };

// The cell up to ROUTE_STEPS steps from a cell toward the goal along the path grid.
[[nodiscard]] auto Waypoint(PathGrid const& path, GridCell from, GridCell goal) -> GridCell {
  auto cell{ from };
  for ([[maybe_unused]] auto const step : std::views::iota(uint8_t{ 0 }, ROUTE_STEPS)) {
    if (cell == goal) break;
    cell = opeu_grid_step_toward(path, cell, goal).cell;
  }
  return cell;
}

// The current unit's move: it plans a route to its order destination and steps along it while it can.
class UnitMove {
public:
  UnitMove(Game& game, bool repeat)
      : _game{ game }, _data{ Data(game) }, _current{ _data.current_unit_ptr }, _unit{ Record<Unit>(_data, _current) }, _path{ FarPathGrid() }, _repeat{ repeat }, _next{ _unit.x, _unit.y },
        _goal_reserve{ GOAL_RESERVE }, _rating{ static_cast<uint8_t>(opeu_move_rating(game)) } { }

  void Run() {
    opeu_add_byte(_rating, POINTS_LIMIT, &_unit.moved);
    if (!CanAffordAStep()) return;
    for (;;) switch (Plan()) {
      case Verdict::STOP:  return;
      case Verdict::RETRY: continue;
      case Verdict::GO:
        if (Step() == Verdict::STOP) return;
        break;
      }
  }

private:
  enum class Verdict {
    GO,
    RETRY,
    STOP,
  };

  [[nodiscard]] auto CostAt(GridCell cell) -> uint8_t {
    return opeu_move_cost(_game, cell.x, cell.y, _current);
  }

  // Whether some neighbouring cell costs no more than the unit's movement points.
  [[nodiscard]] auto CanAffordAStep() -> bool {
    return std::ranges::any_of(std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS), [&](uint8_t direction) {
      auto const cell{ StepOnMap(_next, direction) };
      if (!cell) return false;
      auto const cost{ CostAt(*cell) };
      return cost != IMPASSABLE_COST && cost <= _unit.moved;
    });
  }

  // Plans the route to a waypoint when the unit has none or is close to it, or confirms the route to it once.
  [[nodiscard]] auto Plan() -> Verdict {
    if (_unit.target_x == NONE || (!_committed && GridDistance(_next, { _unit.target_x, _unit.target_y }) < NEAR_DESTINATION)) return PlanToDestination();
    if (_committed) return Verdict::GO;
    if (!opeu_move_reachable(_game, _path,
                             {
                                 { _unit.target_x, _unit.target_y },
                                 _next, _current
    })) {
      _unit.target_x = NONE;
      _shown         = false;
      return Verdict::RETRY;
    }
    _committed = true;
    return Verdict::GO;
  }

  [[nodiscard]] auto PlanToDestination() -> Verdict {
    auto const goal{ Destination() };
    if (!opeu_move_reachable(_game, _path, { goal, _next, _current })) {
      auto& link{ _data.unit_links[_data.current_side * GROUPS_PER_SIDE + _data.current_group] };
      if (!link || opeu_category_6(static_cast<uint8_t>(_data.current_entry))) link = _current;
      _unit.target_x = NONE;
      return Verdict::STOP;
    }
    auto const waypoint{ Waypoint(_path, _next, goal) };
    _unit.target_x = waypoint.x;
    _unit.target_y = waypoint.y;
    _committed     = true;
    return Verdict::GO;
  }

  // The order destination, read through the DS bytes that keep their stale value for an order without a place.
  [[nodiscard]] auto Destination() -> GridCell {
    auto const x_at{ _goal_reserve.At(GOAL_X_AT) };
    auto const y_at{ _goal_reserve.At(GOAL_Y_AT) };
    opeu_unit_order_destination(_game, _current, _game.State().Bytes(x_at, 1).data(), _game.State().Bytes(y_at, 1).data());
    return { Byte(_game, x_at), Byte(_game, y_at) };
  }

  // Whether an enemy objective or unit holds the cell.
  [[nodiscard]] auto Blocked(GridCell cell) -> bool {
    auto const objective{ opeu_find_objective_at(_game, cell.x, cell.y) };
    if (objective != 0 && static_cast<uint8_t>(opeu_objective_side(_game, objective)) != _data.current_side) return true;
    auto const other{ opeu_find_unit_at(_game, cell.x, cell.y) };
    return other != 0 && static_cast<uint8_t>(opeu_unit_side(other)) != _data.current_side;
  }

  // One step along the route: GO to go on, RETRY after an enemy cell barred the way, STOP when the move ends.
  [[nodiscard]] auto Step() -> Verdict {
    _path.Clear(_next);
    auto const toward{ opeu_grid_step_toward(_path, _next, { _unit.target_x, _unit.target_y }) };
    _next      = toward.cell;
    _remaining = toward.cost;
    if (_remaining == STUCK || (!_repeat && !_remaining)) return Verdict::STOP;
    if (Blocked(_next)) {
      _path.Clear(_next);
      _next = { _unit.x, _unit.y };
      return Verdict::GO;
    }
    auto const cost{ CostAt(_next) };
    _unit.moved = static_cast<uint8_t>(opeu_min16(opeu_max16(cost, _rating), _unit.moved));
    if (_unit.moved < cost || static_cast<uint8_t>(opeu_cell_side(_game, _next.x, _next.y)) != NONE) return Verdict::STOP;
    if (!opeu_move_supplied(_game)) {
      _unit.moved = 0;
      return Verdict::STOP;
    }
    return Enter(cost);
  }

  // Moves the unit into _next, paying the cost, and meets what lies there.
  [[nodiscard]] auto Enter(uint8_t cost) -> Verdict {
    opeu_arrival_location(_game, _unit.x, _unit.y, static_cast<uint8_t>(_data.current_side));
    if (!_shown) {
      opeu_arrival_view(_game, _current);
      _shown = true;
    }
    GridCell const from{ _unit.x, _unit.y };
    opeu_unit_remove_draw(_game, _current);
    _unit.x      = _next.x;
    _unit.y      = _next.y;
    _unit.moved -= cost;
    opeu_arrival_prepare(_game, _current);
    opeu_move_clamp(_game);
    if (opeu_cell_marked(_game, _next.x, _next.y, NONE)) {
      opeu_move_mines(_game, from.x, from.y);
      if (!_unit.first) return Verdict::STOP;
    } else {
      opeu_move_spot(_game);
    }
    opeu_arrival_prepare(_game, _current);
    opeu_move_consume(_game);
    if (opeu_move_enemy_adjacent(_game) || !_remaining) return Verdict::STOP;
    return Verdict::GO;
  }

  Game&               _game;
  DataSegment&        _data;
  uint16_t const      _current;
  Unit&               _unit;
  PathGrid const      _path;
  bool const          _repeat;
  GridCell            _next;
  ServiceLocals const _goal_reserve;
  uint8_t const       _rating;
  bool                _shown{ false };     // the arrival view has been shown
  bool                _committed{ false }; // the route is planned or confirmed
  uint16_t            _remaining{ 0 };
};
} // namespace

// repeat: keep stepping while the steps cost nothing.
void opeu_move_unit(Game& game, uint16_t repeat) {
  UnitMove{ game, repeat != 0 }.Run();
}
