// Spotting while the current unit moves: it spots enemy elements within sight (the weather's range, halved at night),
// and the nearest selectable enemy unit may spot its elements in turn. Also: whether a spotted enemy unit stands next
// to the current unit, and the end of the minefield report.
#include "map/movespot.hpp"
#include "common/data-segment.hpp"
#include "common/element-chain.hpp"
#include "common/map-grid.hpp"
#include "common/memory.hpp"
#include "core/sideutil.hpp"
#include "io/waitkey.hpp"
#include "map/distance.hpp"
#include "map/movesup.hpp"
#include "map/occupy.hpp"
#include "map/query.hpp"
#include "map/stepmap.hpp"
#include "map/support.hpp"
#include "types.hpp"
#include "units/stats.hpp"
#include "units/value.hpp"

#include "common/game.hpp"
#include "link/glue.hpp"
#include "port.hpp"

#include <algorithm>
#include <ranges>
#include <span>

using namespace opeu::game;

namespace {
constexpr uint8_t NONE{ 255 };
constexpr uint8_t MINES_REQUEST{ 3 }; // the pending request the minefield report finishes
} // namespace

namespace {
// The current unit spots the elements of each ready enemy unit within sight; returns the distance of the nearest
// selectable enemy unit (NONE when there is none).
[[nodiscard]] auto SpotEnemies(Game& game, uint8_t sight) -> uint8_t {
  auto&      data{ Data(game) };
  auto const current{ data.current_unit_ptr };
  auto const enemy{ static_cast<uint8_t>(opeu_opposite_side(static_cast<uint8_t>(data.current_side))) };
  auto const from{
    GridCell{ Record<Unit>(data, current).x, Record<Unit>(data, current).y }
  };
  uint8_t nearest{ NONE };
  for (auto& other : std::span{ data.units }.subspan(enemy * UNITS_PER_SIDE, UNITS_PER_SIDE)) {
    if (!(other.flags & Bits(UnitFlag::READY))) continue;
    auto const distance{ GridDistance(from, { other.x, other.y }) };
    if (opeu_unit_selection_eligible(game, current) && sight > distance)
      for (auto& element : Elements(data, other))
        if (port_rand_bounded(game, sight) >= distance) element.flags |= Bits(ElementFlag::SPOTTED);
    if (opeu_unit_selection_eligible(game, Near(data, other)) && distance < nearest) nearest = distance;
  }
  return nearest;
}
} // namespace

void opeu_move_spot(Game& game) {
  auto&      data{ Data(game) };
  auto&      mover{ Record<Unit>(data, data.current_unit_ptr) };
  auto const sight{ static_cast<uint8_t>(data.sight_ranges[data.weather] / (opeu_hour_is_night(static_cast<uint8_t>(data.hour)) ? 2 : 1)) };
  auto const nearest{ SpotEnemies(game, sight) };
  if (nearest == NONE || (mover.flags & Bits(UnitFlag::SPOTTED))) return;
  // The nearest enemy may spot the mover's elements in turn; an unseen element may slip away.
  for (auto& element : Elements(data, mover)) {
    if (port_rand_bounded(game, sight) >= nearest)
      element.flags |= Bits(ElementFlag::SPOTTED);
    else if (!port_rand_bounded(game, static_cast<std::uint16_t>(sight * 10)))
      element.flags &= ~Bits(ElementFlag::SPOTTED);
  }
}

uint16_t opeu_move_enemy_adjacent(Game& game) {
  auto const& data{ Data(game) };
  auto const& mover{ Record<Unit>(data, data.current_unit_ptr) };
  auto const  enemy{ static_cast<uint8_t>(opeu_opposite_side(static_cast<uint8_t>(data.current_side))) };
  auto const  spotted_enemy{ [&](uint8_t direction) {
    auto const next{ StepOnMap({ mover.x, mover.y }, direction) };
    return next && static_cast<uint8_t>(opeu_cell_side(game, next->x, next->y)) == enemy && opeu_unit_special(game, static_cast<std::uint16_t>(opeu_find_unit_at(game, next->x, next->y)));
  } };
  return std::ranges::any_of(std::views::iota(uint8_t{ 0 }, GRID_DIRECTIONS), spotted_enemy) ? 1 : 0;
}

void opeu_move_mines_finish(Game& game) {
  opeu_pending_finish(game, static_cast<uint8_t>(MINES_REQUEST));
}
