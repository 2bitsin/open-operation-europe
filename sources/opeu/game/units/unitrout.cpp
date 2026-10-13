// The hourly morale checks of the regiments. opeu_unit_fall_back orders a regiment to fall back to a cell, first
// restoring a saved order. opeu_unit_morale_check(side, group, index) skips regiments of a player-controlled side
// unless their division is marked for the computer. It scans the eight neighbours: a friendly objective or regiment
// ends the scan, hostile ones mark the regiment as threatened. An unthreatened regiment falls back in place when any
// element's experience reaches rand(20) + 60; a threatened one when it reaches rand(10) + 80, preferring a free,
// passable, non-hostile neighbour unless the regiment stands on map code 1 or the element is at 99. Regiments in the
// line slots without a saved order then attack (with chance 1/2) a hostile neighbouring objective and, off map code 1,
// a special enemy regiment next to it.
#include "units/unitrout.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/forces.hpp"
#include "common/game.hpp"
#include "core/recindex.hpp"
#include "core/sideutil.hpp"
#include "map/grid.hpp"
#include "map/movecost.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "types.hpp"
#include "units/orders.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

using namespace opeu::game;

namespace {
// The order codes the checks give.
enum OrderCode : std::uint8_t {
  ATTACK_OBJECTIVE = 1,
  ATTACK_UNIT      = 2,
  FALL_BACK        = 13,
};
constexpr std::uint8_t NO_SAVED_ORDER{ 0xff }; // Unit::orders[1].kind when no order is saved
} // namespace

void opeu_unit_fall_back(Game& game, uint16_t unit, uint8_t x, uint8_t y) {
  auto& record{ Record<Unit>(Data(game), unit) };
  if (record.orders[0].kind == FALL_BACK) return;
  if (record.orders[1].kind != NO_SAVED_ORDER) opeu_restore_unit_order(game, unit);
  opeu_set_unit_order(game, unit, 0, x, 1, FALL_BACK, y);
}

void opeu_unit_morale_check(Game& game, uint8_t side, uint8_t group, uint8_t index) {
  auto&         data{ Data(game) };
  auto&         regiment{ data.units[(side * GROUPS_PER_SIDE + group) * UNITS_PER_GROUP + index] };
  auto const    unit{ Near(data, regiment) };
  std::uint8_t  x;
  std::uint8_t  y;
  std::uint8_t  dir;
  std::uint16_t site;
  if (data.side_control[side] == SIDE_PLAYED && !(GroupOf(data, side, group).flags & Bits(GroupFlag::AI_MARK))) return;
  bool calm{ true };
  for (dir = 0; dir < 8; dir++) {
    x = regiment.x;
    y = regiment.y;
    if (opeu_grid_step(&x, &y, dir)) {
      if ((site = opeu_find_objective_at(game, x, y)) != 0) {
        if (opeu_objective_side(game, site) == side) break;
        calm = false;
      }
      if (auto const close{ opeu_find_unit_at(game, x, y) }; close != 0) {
        if (opeu_unit_side(close) == side) break;
        calm = false;
      }
    }
  }
  if (calm) {
    for (auto const& element : Elements(data, regiment))
      if (static_cast<std::uint8_t>(game.State().RandomBounded(20) + 60) <= element.experience) {
        opeu_unit_fall_back(game, unit, regiment.x, regiment.y);
        return;
      }
  } else {
    for (auto const& element : Elements(data, regiment)) {
      if (static_cast<std::uint8_t>(game.State().RandomBounded(10) + 80) > element.experience) continue;
      if (opeu_map_code(game, regiment.x, regiment.y) == 1 || element.experience == 99) {
        opeu_unit_fall_back(game, unit, regiment.x, regiment.y);
        return;
      }
      for (dir = 0; dir < 8; dir++) {
        x = regiment.x;
        y = regiment.y;
        if (opeu_grid_step(&x, &y, dir) && opeu_cell_side(game, x, y) == NO_SIDE && opeu_move_cost(game, x, y, unit) != IMPASSABLE_COST
            && ((site = opeu_find_objective_at(game, x, y)) == 0 || opeu_objective_side(game, site) == side)) {
          opeu_unit_fall_back(game, unit, x, y);
          return;
        }
      }
      opeu_unit_fall_back(game, unit, regiment.x, regiment.y);
      return;
    }
  }
  if ((opeu_category_0_to_3(index) || opeu_category_4_or_5(index)) && regiment.orders[1].kind == NO_SAVED_ORDER && game.State().RandomBounded(2) == 0) {
    for (dir = 0; dir < 8; dir++) {
      x = regiment.x;
      y = regiment.y;
      if (opeu_grid_step(&x, &y, dir) && (site = opeu_find_objective_at(game, x, y)) != 0 && opeu_objective_side(game, site) != side) {
        auto const& objective{ Record<Objective>(data, site) };
        opeu_set_unit_order(game, unit, 0, objective.x, 1, ATTACK_OBJECTIVE, objective.y);
        break;
      }
    }
    if (opeu_map_code(game, regiment.x, regiment.y) != 1) {
      for (dir = 0; dir < 8; dir++) {
        x = regiment.x;
        y = regiment.y;
        if (opeu_grid_step(&x, &y, dir) && opeu_cell_side(game, x, y) == opeu_opposite_side(side)) {
          auto const close{ opeu_find_unit_at(game, x, y) };
          if (opeu_unit_special(game, close)) {
            opeu_set_unit_order(game, unit, close, 0, 1, ATTACK_UNIT, 0);
            break;
          }
        }
      }
    }
  }
}
